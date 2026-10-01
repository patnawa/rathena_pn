"""Bounded static Zeny vendor edges for resource-cycle screening.

Prices follow the native final-row Buy/Sell presence rule, item price guard,
and maximum ordinary-player skills (25% buy discount, 24% overcharge). Purchases
of NoSell materials are retained. Market stock is deliberately relaxed to
unlimited, including currently empty markets. NPC accessibility, bindings,
cross-character transfer, SQL/dynamic prices and purchase/sale script callbacks
are not execution proofs. An edge is an optimistic source-derived opportunity.
"""
import re
import audit_item_acquisition as acquisition


MAX_ZENY = 2147483647


def effective_items(records):
    items = {}
    for source, row in records:
        iid = row['Id']
        item = items.setdefault(iid, {'buy': 0, 'sell': 0, 'row': {}, 'nosell': False})
        acquisition.overlay(item['row'], row)
        trade = row.get('Trade')
        if isinstance(trade, dict) and 'NoSell' in trade:
            if type(trade['NoSell']) is not bool:
                raise ValueError('Invalid NoSell flag: ' + str(iid))
            item['nosell'] = trade['NoSell']
        for name in ('Buy', 'Sell'):
            if name in row:
                value = row[name]
                if type(value) is not int or value < 0:
                    raise ValueError('Invalid item price: ' + str(iid))
                item[name.lower()] = min(value, MAX_ZENY)
        # Native hasPriceValue is overwritten for each parsed row, even when
        # that overlay contains no price fields. Merging presence is incorrect.
        item['has_buy'], item['has_sell'] = 'Buy' in row, 'Sell' in row
        item['source'] = source
    for item in items.values():
        if item['has_buy'] and not item['has_sell']:
            item['sell'] = item['buy'] // 2
        elif item['has_sell'] and not item['has_buy']:
            item['buy'] = item['sell'] * 2
        if item['buy'] > MAX_ZENY:
            raise ValueError('Derived item Buy exceeds native signed shop-price range')
        if item['buy'] * 75 < item['sell'] * 124:
            item['sell'] = 1
    return items


def parse_shop_line(line, items, source, line_number, minimum_buy):
    parts = line.split('\t')
    if len(parts) < 4 or parts[1] not in ('shop', 'marketshop'):
        return []
    location, kind, name = parts[:3]
    fields = parts[3].split(',')[1:]
    discount = kind == 'shop'
    if fields and fields[0].strip().lower() in ('yes', 'no'):
        if kind != 'shop':
            raise ValueError('Unsupported market discount declaration')
        discount = fields.pop(0).strip().lower() == 'yes'
    offers = []
    for index, token in enumerate(fields):
        # Native ordinary shops scan only two integer fields and then advance
        # to the next comma. Preserve ignored suffixes as diagnostics. Markets
        # require exactly three fields here so missing-comma defects stay loud.
        pattern = r'(\d+):(-?\d+)' + (r':(-?\d+)' if kind == 'marketshop' else r'(.*)')
        match = re.fullmatch(pattern, token.strip())
        if not match:
            raise ValueError('Unsupported vendor token: ' + token.strip())
        iid, price = int(match[1]), int(match[2])
        if iid not in items:
            raise ValueError('Unknown vendor item: ' + str(iid))
        if not -2147483648 <= price <= MAX_ZENY:
            raise ValueError('Vendor price outside native int32 range')
        if price < 0:
            price = items[iid]['buy']
        cost = max(minimum_buy, price * 75 // 100) if discount else price
        stock = int(match[3]) if kind == 'marketshop' else None
        if stock is not None and not -2147483648 <= stock <= MAX_ZENY:
            raise ValueError('Vendor stock outside native int32 range')
        if stock is not None and stock < -1:
            stock = -1
        offers.append(dict(source=source, line=line_number, shop=name, kind=kind,
            location=location, item_id=iid, configured_price=price, discount=discount,
            minimum_cost=cost, stock=stock, stock_relaxed=stock is not None and stock >= 0,
            declaration_index=index, native_ignored_suffix=match[3] if kind == 'shop' else ''))
    return offers


def vendor_actions(offers, items, resource_ids, minimum_sell):
    actions = []
    seen = set()
    for offer in offers:
        key = f"buy:{offer['shop']}:{offer['line']}:{offer['item_id']}"
        if key in seen:
            # Preserve all duplicate opportunities as an upper bound rather
            # than silently guessing the native duplicate/override behavior.
            key += ':' + str(offer.get('declaration_index', 0))
        if key in seen:
            raise ValueError('Duplicate vendor action identity: ' + key)
        seen.add(key)
        actions.append(dict(key=key, net={'zeny': -offer['minimum_cost'],
            'item:' + str(offer['item_id']): 1}, source=offer['source'], line=offer['line'],
            kind='vendor_buy', offer=offer))
    for iid in sorted(resource_ids):
        if iid not in items:
            raise ValueError('Undefined recipe/vendor resource: ' + str(iid))
        item = items[iid]
        if item['nosell']:
            continue
        revenue = max(minimum_sell, item['sell'] * 124 // 100)
        if revenue:
            actions.append(dict(key='sell:' + str(iid), net={'zeny': revenue, 'item:' + str(iid): -1},
                                kind='vendor_sell', source=item.get('source', 'item database'), line=None))
    return actions


def load_vendors(root=acquisition.REPO):
    previous_root, previous_hashes = acquisition.REPO, acquisition.hashes
    acquisition.REPO, acquisition.hashes = root, {}
    try:
        items = effective_items(acquisition.records('db/item_db.yml'))
        active, configs = set(), set()
        def include(path):
            if path in configs:
                return
            configs.add(path)
            for kind, target in re.findall(r'^(npc|import):\s*([^\s]+)',
                    acquisition.strip_comments(acquisition.read(path)), re.M):
                if kind == 'import':
                    include(target)
                elif (root / target).exists():
                    active.add(target)
        include('npc/re/scripts_main.conf')
        limits = {'min_shop_buy': 1, 'min_shop_sell': 0}
        visiting = set()
        def battle(path):
            if path in visiting:
                raise ValueError('Battle configuration import cycle: ' + path)
            if not (root / path).exists():
                return
            visiting.add(path)
            for name, value in re.findall(r'^\s*(import|min_shop_buy|min_shop_sell):\s*([^\s]+)',
                    acquisition.strip_comments(acquisition.read(path)), re.M):
                if name == 'import':
                    battle(value)
                else:
                    limits[name] = int(value)
                    if not 0 <= limits[name] <= MAX_ZENY:
                        raise ValueError('Unsupported shop minimum configuration')
            visiting.remove(path)
        battle('conf/battle_athena.conf')
        offers, unsupported = [], []
        for source in sorted(active):
            for number, line in enumerate(acquisition.strip_comments(acquisition.read(source)).splitlines(), 1):
                try:
                    offers.extend(parse_shop_line(line, items, source, number, limits['min_shop_buy']))
                except ValueError as error:
                    unsupported.append(dict(source=source, line=number, reason=str(error), declaration=line.strip()))
        # Bind the hard-coded ordinary-skill envelope to effective source data,
        # and refuse silently changing it after a skill-level customization.
        skills = {}
        for source, row in acquisition.records('db/skill_db.yml'):
            acquisition.overlay(skills.setdefault(row['Id'], {}), row)
        levels = {row.get('Name'):row.get('MaxLevel') for row in skills.values()}
        for name, expected in (('MC_DISCOUNT', 10), ('MC_OVERCHARGE', 10), ('RG_COMPULSION', 5)):
            if levels.get(name) != expected:
                raise ValueError('Unsupported price skill level: ' + name)
        constants = acquisition.read('src/common/mmo.hpp')
        if not re.search(r'^#define MAX_ZENY INT_MAX\b', constants, re.M):
            raise ValueError('Unsupported native Zeny price limit')
        for path in ('src/map/itemdb.cpp', 'src/map/pc.cpp', 'src/map/npc.cpp'):
            acquisition.read(path)
        return dict(items=items, offers=offers, unsupported=unsupported,
                    active_npc_files=len(active), limits=limits, source_sha256=dict(acquisition.hashes), boundary=__doc__)
    finally:
        acquisition.REPO, acquisition.hashes = previous_root, previous_hashes
