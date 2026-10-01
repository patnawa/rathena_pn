"""Conservative resource-cycle screen for the validated typed reward catalog.

Use maximum possible outputs, even for rare draws. Iteratively remove a recipe
that consumes a resource no remaining recipe can replenish. This proves the
removed recipes cannot participate in a self-funding combination within this
closed model. Retained recipes are candidates, never confirmed profit loops.
With --vendors, add static Zeny purchases and item sales, retaining NoSell buys
and relaxing finite market stock. With --solve, an optional SciPy search proposes
a strictly positive resource valuation or growth combination. Every proposed
result is verified using exact rational arithmetic before it changes the verdict.
Missing dynamic paths, reset paths and eligibility still need native confirmation
before making whole-economy claims. Rare outputs use their simultaneous maxima;
a verified model witness is a runtime-review candidate, never a proven exploit.
"""
import argparse
from collections import defaultdict
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import dynamic_reward_catalog as catalog


def recipe_actions(recipes):
    actions=[];excluded=[];unpriced=[];seen=set()
    for row in recipes:
        key=row['family']+':'+str(row['key'])
        if key in seen:raise ValueError('Duplicate recipe identity: '+key)
        seen.add(key)
        if row['repeat']!='repeatable':
            excluded.append({'recipe':key,'reason':row['repeat']});continue
        net=defaultdict(int)
        for cost in row['costs']:
            if cost['item_id']<=0 or cost['amount']<=0:raise ValueError('Invalid material cost')
            net['item:'+str(cost['item_id'])]-=cost['amount']
        zeny=row.get('zeny_per_batch',0)
        if not isinstance(zeny,int) or zeny<0:raise ValueError('Invalid Zeny cost')
        net['zeny']-=zeny
        for output in row['outputs']:
            low,high,chance=output['minimum'],output['maximum'],output['chance_per_100000']
            if output['item_id']<=0 or not 0<=low<=high or not 0<=chance<=100000:raise ValueError('Invalid output range')
            if chance:net['item:'+str(output['item_id'])]+=high
        if not row['costs'] and not zeny:unpriced.append(key)
        actions.append(dict(key=key,net={k:v for k,v in net.items() if v},source=row['source'],
                     line=row['line'],condition=row.get('condition',''),kind='recipe'))
    return actions,dict(excluded=excluded,unpriced=unpriced)


def combined_actions(recipes, vendors):
    actions,details=recipe_actions(recipes)
    actions.extend(vendors)
    if len({row['key'] for row in actions})!=len(actions):
        raise ValueError('Duplicate action identity')
    return actions,details


def action_core(actions):
    active={row['key']:{k:v for k,v in row.items() if k!='key'} for row in actions}
    rounds=[]
    while active:
        produced={resource for row in active.values() for resource,amount in row['net'].items() if amount>0}
        removed=[{'recipe':key,'unreplenished':sorted(k for k,v in row['net'].items() if v<0 and k not in produced)} for key,row in active.items()]
        removed=[row for row in removed if row['unreplenished']]
        if not removed:break
        rounds.append(removed)
        for row in removed:del active[row['recipe']]
    return dict(elimination_rounds=rounds,candidate_core=active)


def cycle_core(recipes):
    actions,details=recipe_actions(recipes)
    report=action_core(actions)
    return dict(modeled_recipes=len(actions),**details,**report,
                verdict='requires_runtime_review' if report['candidate_core'] else 'no_closed_cycle_in_modeled_recipes')


def rational(value):
    # Fraction rejects NaN and infinity. Reject booleans as malformed input.
    if isinstance(value,bool):raise ValueError('Boolean coefficient')
    return Fraction(str(value))


def rational_text(value):
    return str(value.numerator) if value.denominator==1 else str(value)


def resources(actions):
    return sorted({resource for row in actions for resource,amount in row['net'].items() if amount})


def action_digest(actions):
    # Only the resource vectors and identities define the closed model. Source
    # evidence and optimistic eligibility notes stay separately inspectable.
    rows=sorted(({'key':row['key'],'net':{name:value for name,value in sorted(row['net'].items()) if value}}
                 for row in actions),key=lambda row:row['key'])
    return hashlib.sha256(json.dumps(rows,sort_keys=True,separators=(',',':')).encode()).hexdigest()


def verify_no_growth_certificate(actions, potentials):
    """Strictly positive valuation with every action's total value <= 0.

    Such a valuation excludes every nonnegative aggregate resource vector with
    any positive component in this closed action model, including item growth.
    Zero valuations are insufficient because they can conceal item-only growth.
    """
    try:
        names=resources(actions)
        if set(potentials)!=set(names):raise ValueError('Certificate must value every modeled resource')
        values={name:rational(potentials[name]) for name in names}
        if any(value<=0 for value in values.values()):raise ValueError('Resource valuations must be strictly positive')
        violations=[]
        for row in actions:
            delta=sum((amount*values[name] for name,amount in row['net'].items() if amount),Fraction())
            if delta>0:violations.append(dict(action=row['key'],increase=rational_text(delta)))
        return dict(verified=not violations,violations=violations,
                    potentials={name:rational_text(value) for name,value in values.items()})
    except (ValueError,TypeError,ZeroDivisionError) as error:
        return dict(verified=False,error=str(error))


def verify_growth_witness(actions, coefficients):
    """Verify a proposed nonnegative action combination without float tolerance."""
    try:
        by_key={row['key']:row for row in actions}
        if set(coefficients)-set(by_key):raise ValueError('Witness names an unknown action')
        values={key:rational(value) for key,value in coefficients.items()}
        if any(value<0 for value in values.values()):raise ValueError('Negative action coefficient')
        net=defaultdict(Fraction)
        for key,value in values.items():
            for name,amount in by_key[key]['net'].items():net[name]+=value*amount
        net={name:value for name,value in net.items() if value}
        verified=bool(net) and all(value>=0 for value in net.values())
        # Integers remain convenient for reports/tests; fractions are explicit.
        return dict(verified=verified,net={name:int(value) if value.denominator==1 else rational_text(value) for name,value in net.items()},
                    coefficients={key:rational_text(value) for key,value in values.items() if value})
    except (ValueError,TypeError,ZeroDivisionError) as error:
        return dict(verified=False,error=str(error))


def linear_screen(actions):
    """Optional numerical search; only exact verification produces a verdict."""
    try:
        import scipy
        from scipy.optimize import linprog
        from scipy.sparse import coo_matrix
        import numpy as np
    except ImportError:
        return dict(verdict='linear_search_unavailable',reason='Install scipy to use --solve')
    names=resources(actions);index={name:i for i,name in enumerate(names)}
    if not names:
        return dict(verdict='no_resource_growth_in_combined_model',certificate=verify_no_growth_certificate(actions,{}),
                    solver='not_needed',resources=0,actions=len(actions))
    rows=[];columns=[];amounts=[]
    for i,row in enumerate(actions):
        for name,amount in row['net'].items():
            if amount:
                rows.append(i);columns.append(index[name]);amounts.append(amount)
    matrix=coo_matrix((amounts,(rows,columns)),shape=(len(actions),len(names))).tocsr()
    # Normalize Zeny to one where present and require positive item valuations.
    # Fixed 1e-6 may miss an extremely small feasible valuation; failure is
    # unresolved unless the separate primal search yields an exact witness.
    bounds=[(1,1) if name=='zeny' else (1e-6,None) for name in names]
    dual=linprog(np.zeros(len(names)),A_ub=matrix,b_ub=np.zeros(len(actions)),bounds=bounds,method='highs')
    details=dict(solver='scipy.optimize.linprog/highs',scipy_version=scipy.__version__,
                 resources=len(names),actions=len(actions),dual_status=int(dual.status),dual_message=dual.message)
    if dual.success:
        values={name:rational_text(Fraction(str(float(dual.x[i]))).limit_denominator(10**9)) for i,name in enumerate(names)}
        certificate=verify_no_growth_certificate(actions,values)
        if certificate['verified']:
            return dict(verdict='no_resource_growth_in_combined_model',certificate=certificate,**details)
        details['rejected_certificate']=certificate
    # Normalized nonnegative mixtures keep the numerical search bounded. Exact
    # checking distinguishes real growth from rounding-induced near-zero loops.
    objective=-np.asarray(matrix.sum(axis=1)).ravel()
    primal=linprog(objective,A_ub=-matrix.T,b_ub=np.zeros(len(names)),
        A_eq=np.ones((1,len(actions))),b_eq=[1],bounds=(0,None),method='highs')
    details.update(primal_status=int(primal.status),primal_message=primal.message)
    if primal.success:
        proposed={row['key']:rational_text(Fraction(str(float(primal.x[i]))).limit_denominator(10**9))
                  for i,row in enumerate(actions) if primal.x[i]>0}
        witness=verify_growth_witness(actions,proposed)
        if witness['verified']:
            return dict(verdict='combined_model_growth_requires_runtime_review',witness=witness,**details)
        details['rejected_witness']=witness
    return dict(verdict='linear_search_unresolved',**details)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--vendors',action='store_true',help='Combine recipes with optimistic static Zeny vendor edges')
    parser.add_argument('--solve',action='store_true',help='Search combined model with optional SciPy, then verify exactly')
    args=parser.parse_args()
    path=catalog.ROOT/catalog.CATALOG;raw=path.read_bytes();data=json.loads(raw)
    validation=catalog.validate(data)
    report=cycle_core(data['recipes'])
    if args.solve and not args.vendors:parser.error('--solve requires --vendors')
    if args.vendors:
        from economy_vendor_catalog import load_vendors,vendor_actions
        vendors=load_vendors()
        ids={entry['item_id'] for row in data['recipes'] for entry in row['costs']+row['outputs']}
        ids.update(offer['item_id'] for offer in vendors['offers'])
        added=vendor_actions(vendors['offers'],vendors['items'],ids,vendors['limits']['min_shop_sell'])
        actions,details=combined_actions(data['recipes'],added)
        core=action_core(actions)
        report.update(details)
        report.update(combined_actions=len(actions),vendor_actions=len(added),
            combined_model_sha256=action_digest(actions),
            combined_elimination_rounds=core['elimination_rounds'],combined_candidate_core=core['candidate_core'],
            vendor_catalog={key:value for key,value in vendors.items() if key not in ('items','offers')},
            offers_screened=len(vendors['offers']),nosell_buy_offers=sum(vendors['items'][offer['item_id']]['nosell'] for offer in vendors['offers']),
            verdict='combined_candidates_require_runtime_review' if core['candidate_core'] else 'no_closed_cycle_in_combined_model')
        if args.solve:
            # Use every action, including eliminated actions, so a certificate
            # directly values and verifies the complete bounded model.
            linear=linear_screen(actions)
            report.update(linear_screen=linear,verdict=linear['verdict'])
    report.update(catalog_sha256=hashlib.sha256(raw).hexdigest(),source_sha256=data['source_sha256'],
                  catalog_validation=validation,boundary=__doc__,
                  tool_sha256={path.relative_to(catalog.ROOT).as_posix():hashlib.sha256(path.read_bytes()).hexdigest()
                    for path in (Path(__file__),Path(__file__).with_name('economy_vendor_catalog.py'),
                                 Path(__file__).with_name('audit_item_acquisition.py'),
                                 Path(__file__).with_name('dynamic_reward_catalog.py'))})
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key] for key in ('modeled_recipes','verdict')}))
    print('Candidate core:',len(report.get('combined_candidate_core',report['candidate_core'])),
          'Excluded finite/unknown repeatability:',len(report['excluded']))


if __name__=='__main__':main()
