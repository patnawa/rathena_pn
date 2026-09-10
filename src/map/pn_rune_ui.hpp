// Project contributions: (C) 2026 PN Development Team.
// GPL-3.0-or-later; see LICENSE. Existing upstream rights retained.
// Source: https://github.com/patnawa/rathena_pn/blob/main/src/map/pn_rune_ui.hpp

// Native Rune Tablet window bridge. Wire layouts verified against the client.
// The existing workshop remains the sole owner of inventory/progression changes.
#ifndef PN_RUNE_UI_HPP
#define PN_RUNE_UI_HPP
struct pn_rune_ui_entry { uint16 tag; uint32 id; bool piece; uint32 pity_step[15]; };
static constexpr pn_rune_ui_entry pn_rune_ui_catalog[] = {
    {17, 1263000, true, {0}},
    {17, 1263001, true, {0}},
    {17, 1263002, true, {0}},
    {17, 1263003, true, {0}},
    {17, 1263004, true, {0}},
    {17, 1263005, true, {0}},
    {17, 1263006, true, {0}},
    {17, 1263007, true, {0}},
    {17, 1263008, true, {0}},
    {17, 1263009, true, {0}},
    {17, 1263010, true, {0}},
    {17, 1263011, true, {0}},
    {17, 1263012, true, {0}},
    {17, 1263013, true, {0}},
    {17, 1263014, true, {0}},
    {17, 1263015, true, {0}},
    {17, 1263016, true, {0}},
    {17, 1263017, true, {0}},
    {17, 1263018, true, {0}},
    {17, 1263019, true, {0}},
    {17, 1263020, true, {0}},
    {17, 1263021, true, {0}},
    {17, 1263022, true, {0}},
    {17, 1263023, true, {0}},
    {17, 1263024, true, {0}},
    {17, 1263025, true, {0}},
    {17, 1263026, true, {0}},
    {17, 1263027, true, {0}},
    {17, 1263028, true, {0}},
    {17, 1263029, true, {0}},
    {17, 1263030, true, {0}},
    {18, 1263035, true, {0}},
    {18, 1263036, true, {0}},
    {18, 1263037, true, {0}},
    {18, 1263038, true, {0}},
    {18, 1263039, true, {0}},
    {18, 1263040, true, {0}},
    {18, 1263041, true, {0}},
    {18, 1263042, true, {0}},
    {18, 1263043, true, {0}},
    {18, 1263044, true, {0}},
    {18, 1263045, true, {0}},
    {18, 1263046, true, {0}},
    {18, 1263047, true, {0}},
    {18, 1263048, true, {0}},
    {18, 1263049, true, {0}},
    {18, 1263050, true, {0}},
    {18, 1263051, true, {0}},
    {18, 1263052, true, {0}},
    {18, 1263053, true, {0}},
    {19, 1263054, true, {0}},
    {19, 1263055, true, {0}},
    {19, 1263056, true, {0}},
    {19, 1263057, true, {0}},
    {19, 1263058, true, {0}},
    {19, 1263059, true, {0}},
    {19, 1263060, true, {0}},
    {19, 1263061, true, {0}},
    {19, 1263062, true, {0}},
    {19, 1263063, true, {0}},
    {19, 1263064, true, {0}},
    {19, 1263065, true, {0}},
    {19, 1263066, true, {0}},
    {19, 1263067, true, {0}},
    {19, 1263068, true, {0}},
    {19, 1263069, true, {0}},
    {19, 1263070, true, {0}},
    {20, 1263080, true, {0}},
    {20, 1263081, true, {0}},
    {20, 1263082, true, {0}},
    {20, 1263083, true, {0}},
    {20, 1263084, true, {0}},
    {20, 1263085, true, {0}},
    {20, 1263086, true, {0}},
    {20, 1263087, true, {0}},
    {20, 1263088, true, {0}},
    {20, 1263089, true, {0}},
    {20, 1263090, true, {0}},
    {20, 1263091, true, {0}},
    {20, 1263092, true, {0}},
    {21, 1263106, true, {0}},
    {21, 1263107, true, {0}},
    {21, 1263108, true, {0}},
    {21, 1263109, true, {0}},
    {21, 1263110, true, {0}},
    {21, 1263111, true, {0}},
    {21, 1263112, true, {0}},
    {21, 1263113, true, {0}},
    {21, 1263114, true, {0}},
    {21, 1263115, true, {0}},
    {21, 1263116, true, {0}},
    {21, 1263117, true, {0}},
    {21, 1263118, true, {0}},
    {21, 1263119, true, {0}},
    {21, 1263120, true, {0}},
    {22, 1263093, true, {0}},
    {22, 1263094, true, {0}},
    {22, 1263095, true, {0}},
    {22, 1263096, true, {0}},
    {22, 1263097, true, {0}},
    {22, 1263098, true, {0}},
    {22, 1263099, true, {0}},
    {22, 1263100, true, {0}},
    {22, 1263101, true, {0}},
    {22, 1263102, true, {0}},
    {22, 1263103, true, {0}},
    {22, 1263104, true, {0}},
    {22, 1263105, true, {0}},
    {500, 1263031, true, {0}},
    {500, 1263032, true, {0}},
    {500, 1263033, true, {0}},
    {500, 1263034, true, {0}},
    {500, 1263072, true, {0}},
    {500, 1263073, true, {0}},
    {500, 1263074, true, {0}},
    {500, 1263075, true, {0}},
    {500, 1263076, true, {0}},
    {500, 1263077, true, {0}},
    {500, 1263078, true, {0}},
    {500, 1263079, true, {0}},
    {17, 1260008, false, {4000,4000,4000,3600,3600,3200,2800,2400,2000,1600,1200,800,400,200,40}},
    {17, 1260009, false, {4000,4000,4000,3600,3600,3200,2800,2400,2000,1600,1200,800,400,200,40}},
    {17, 1260010, false, {4000,4000,4000,3600,3600,3200,2800,2400,2000,1600,1200,800,400,200,40}},
    {17, 1260011, false, {4000,4000,4000,3600,3600,3200,2800,2400,2000,1600,1200,800,400,200,40}},
    {17, 1260012, false, {3000,3000,3000,2500,2500,2000,1500,1000,900,800,600,500,300,100,20}},
    {17, 1260000, false, {4000,4000,4000,3600,3600,3200,2800,2400,2000,1600,1200,800,400,200,40}},
    {17, 1260001, false, {4000,4000,4000,3600,3600,3200,2800,2400,2000,1600,1200,800,400,200,40}},
    {17, 1260002, false, {4000,4000,4000,3600,3600,3200,2800,2400,2000,1600,1200,800,400,200,40}},
    {17, 1260003, false, {4000,4000,4000,3600,3600,3200,2800,2400,2000,1600,1200,800,400,200,40}},
    {17, 1260004, false, {3000,3000,3000,2500,2500,2000,1500,1000,900,800,600,500,300,100,20}},
    {17, 1260005, false, {4000,4000,4000,3600,3600,3200,2800,2400,2000,1600,1200,800,400,200,40}},
    {17, 1260006, false, {4000,4000,4000,3600,3600,3200,2800,2400,2000,1600,1200,800,400,200,40}},
    {17, 1260007, false, {4000,4000,4000,3600,3600,3200,2800,2400,2000,1600,1200,800,400,200,40}},
    {18, 1260014, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {18, 1260015, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {18, 1260016, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {18, 1260017, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {18, 1260018, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {18, 1260019, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {18, 1260020, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {18, 1260021, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {18, 1260022, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {19, 1260023, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {19, 1260024, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {19, 1260025, false, {7200,7200,7200,6300,6300,5400,4500,3600,2700,1800,900,450,450,220,40}},
    {19, 1260026, false, {7200,7200,7200,6300,6300,5400,4500,3600,2700,1800,900,450,450,220,40}},
    {19, 1260027, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {19, 1260028, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {19, 1260029, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {19, 1260030, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {19, 1260031, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {19, 1260032, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {20, 1260039, false, {7200,7200,7200,6300,6300,5400,4500,3600,2700,1800,900,450,450,220,40}},
    {20, 1260040, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {20, 1260041, false, {7200,7200,7200,6300,6300,5400,4500,3600,2700,1800,900,450,450,220,40}},
    {20, 1260042, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {20, 1260043, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {20, 1260037, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {20, 1260038, false, {7200,7200,7200,6300,6300,5400,4500,3600,2700,1800,900,450,450,220,40}},
    {20, 1260044, false, {7200,7200,7200,6300,6300,5400,4500,3600,2700,1800,900,450,450,220,40}},
    {21, 1260053, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {21, 1260054, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {21, 1260055, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {21, 1260056, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {21, 1260057, false, {7200,7200,7200,6300,6300,5400,4500,3600,2700,1800,900,450,450,220,40}},
    {21, 1260048, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {21, 1260049, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {21, 1260050, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {21, 1260051, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {21, 1260052, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {22, 1260047, false, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
    {22, 1260045, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {22, 1260046, false, {9000,9000,9000,8100,8100,7200,6300,5400,4500,3600,2700,1800,900,450,90}},
    {500, 1260035, false, {0,0,0,2200,2200,2200,2200,2200,2200,2200,2200,2200,2200,2200,2200}},
    {500, 1260036, false, {0,0,0,2200,2200,2200,2200,2200,2200,2200,2200,2200,2200,2200,2200}},
    {500, 1260013, false, {0,0,0,2100,2100,2100,2100,2100,2100,2100,2100,2100,2100,2100,2100}},
    {500, 1260034, false, {0,0,0,2200,2200,2200,2200,2200,2200,2200,2200,2200,2200,2200,2200}},
};
static int64 pn_rune_value(map_session_data* sd, const char* name, uint32 index = 0) {
    return pc_readregistry(sd, reference_uid(add_str(name), index));
}
static void pn_rune_flag(map_session_data* sd, bool opened) {
    pc_setreg(sd, add_str("@PNRTUIOpen"), opened ? 1 : 0);
}
static void pn_rune_control(map_session_data* sd, uint16 packet, uint8 value) {
    int fd = sd->fd;
    WFIFOHEAD(fd,3); WFIFOW(fd,0)=packet; WFIFOB(fd,2)=value; WFIFOSET(fd,3);
}
static const pn_rune_ui_entry* pn_rune_find(uint16 tag, uint32 id) {
    for (const auto& e : pn_rune_ui_catalog) if (e.tag==tag && e.id==id) return &e;
    return nullptr;
}
static bool pn_rune_tag(uint16 tag) {
    for (const auto& e : pn_rune_ui_catalog) if (e.tag==tag) return true;
    return false;
}
static void pn_rune_list(map_session_data* sd, uint16 tag, bool pieces) {
    std::vector<const pn_rune_ui_entry*> owned;
    for (const auto& e : pn_rune_ui_catalog) {
        if (e.tag!=tag || e.piece!=pieces) continue;
        if (pn_rune_value(sd, pieces ? "#PNRTPiece" : "PNRTPaid", e.id-(pieces?1263000:1260000))) owned.push_back(&e);
    }
    int fd=sd->fd;
    uint16 length=9+owned.size()*(pieces?4:8);
    WFIFOHEAD(fd,length); WFIFOW(fd,0)=pieces?0x0bcc:0x0bcd; WFIFOW(fd,2)=length;
    WFIFOB(fd,4)=0; WFIFOW(fd,5)=tag; WFIFOW(fd,7)=owned.size();
    int offset=9;
    for (const auto* e : owned) {
        WFIFOL(fd,offset)=e->id;
        if (!pieces) {
            WFIFOW(fd,offset+4)=static_cast<uint16>(pn_rune_value(sd,"PNRTLevel",e->id-1260000));
            // The client multiplies this failure count by its per-level pity step.
            int64 level=pn_rune_value(sd,"PNRTLevel",e->id-1260000);
            int64 pity=pn_rune_value(sd,"PNRTPity",e->id-1260000);
            uint32 step=(level>=0 && level<15)?e->pity_step[level]:0;
            WFIFOW(fd,offset+6)=step?static_cast<uint16>((pity+step-1)/step):0;
        }
        offset+=pieces?4:8;
    }
    WFIFOSET(fd,length);
}
static void pn_rune_sync(map_session_data* sd) {
    for (uint16 tag : {17,18,19,20,21,22,500}) {
        pn_rune_list(sd,tag,true); pn_rune_list(sd,tag,false);
    }
    uint32 active=static_cast<uint32>(pn_rune_value(sd,"PNRTActive"));
    for (const auto& e : pn_rune_ui_catalog) if (!e.piece && e.id==active) {
        int fd=sd->fd;
        WFIFOHEAD(fd,9); WFIFOW(fd,0)=0x0bd7; WFIFOB(fd,2)=0;
        WFIFOW(fd,3)=e.tag; WFIFOL(fd,5)=e.id; WFIFOSET(fd,9); break;
    }
}
void clif_rune_ui_open(map_session_data& sd) {
    // 0x0bdf is the dedicated native window command, not generic UI_OPEN.
    pn_rune_flag(&sd,true); pn_rune_control(&sd,0x0bdf,1);
}
void clif_parse_rune_ui(int32 fd, map_session_data* sd) {
    uint8 mode=RFIFOB(fd,2);
    if (mode>1) return;
    if (!mode) { pn_rune_flag(sd,false); pn_rune_control(sd,0x0be1,0); return; }
    if (sd->npc_id || sd->state.trading || sd->state.storage_flag) {
        pn_rune_flag(sd,false); pn_rune_control(sd,0x0be1,0); return;
    }
    pn_rune_flag(sd,true); pn_rune_control(sd,0x0be1,1); pn_rune_sync(sd);
}
void clif_parse_rune_list(int32 fd, map_session_data* sd) {
    uint16 tag=RFIFOW(fd,2);
    if (!pc_readreg2(sd,"@PNRTUIOpen") || !pn_rune_tag(tag)) return;
    pn_rune_list(sd,tag,true); pn_rune_list(sd,tag,false);
}
void clif_parse_rune_action(int32 fd, map_session_data* sd) {
    if (!pc_readreg2(sd,"@PNRTUIOpen") || sd->npc_id || sd->state.trading || sd->state.storage_flag) return;
    uint16 command=RFIFOW(fd,0);
    uint32 id=0; uint16 tag=0;
    if (command!=0x0bd4 && command!=0x0bd8) {
        tag=RFIFOW(fd,2); id=RFIFOL(fd,4);
        const auto* e=pn_rune_find(tag,id);
        if (!e || (command==0x0bce)!=e->piece) return;
    } else if (command==0x0bd4) id=static_cast<uint32>(pn_rune_value(sd,"PNRTActive"));
    pc_setreg(sd,add_str("@PNRTNativeCommand"),command);
    pc_setreg(sd,add_str("@PNRTNativeID"),id);
    pn_rune_flag(sd,false); pn_rune_control(sd,0x0bdf,0);
    npc_event(sd,"PN Rune Stone::OnNativeAction",0);
}
#endif
