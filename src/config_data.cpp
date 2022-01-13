static uint16_t* makeCreatureTileIndex(const vector<Creature *>& clist,
                                       const Tileset* ts,
                                       const UltimaSaveIds& usaveIds) {
    uint16_t* index = new uint16_t[ ts->tileCount ];
    int ci, ccount;
    TileId tid;

    ccount = clist.size();
    for (tid = 0; tid != ts->tileCount; ++tid) {
        for (ci = 0; ci < ccount; ++ci) {
            const Creature* cp = clist[ci];
            if (cp->tile.id == tid)
                break;
            if (cp->u4SaveId) {
                // This supports fire_phantom which is saved as fire_field.
                if (tid == usaveIds.moduleId( cp->u4SaveId ).id)
                    break;
            }
        }
        index[tid] = (ci < ccount) ? ci : 0xffff;
    }
    return index;
}

#if 0
#define DUMP_CONFIG
#define CF_STR(id)   cfg->confString(id)
#define CF_SYM(id)   cfg->symbolName(id)

void dumpArmor(const Config* cfg, Armor* arm) {
    printf( "Armor %d \"%s\" 0x%x %d\n",
            arm->type, CF_STR(arm->name), arm->canuse, arm->defense );
}

void dumpWeapon(const Config* cfg, Weapon* wpn) {
    printf( "Weapon %2d \"%s\" \"%s\" %d %d 0x%x 0x%x (%s %s %s)\n",
            wpn->type, CF_STR(wpn->abbr), CF_STR(wpn->name),
            wpn->range, wpn->damage,
            wpn->canuse, wpn->flags,
            CF_SYM(wpn->hitTile),
            CF_SYM(wpn->missTile),
            CF_SYM(wpn->leaveTile) );
}

void dumpCreature(const Config* cfg, Creature* cr) {
    printf( "Creature %d \"%s\" %d,%d,%d,%d 0x%x 0x%x 0x%x %d,%d,%d"
            " (%s %s %s %s)\n",
            cr->id, CF_STR(cr->name),
            cr->spawn, cr->basehp, cr->xp, cr->encounterSize,
            cr->mattr, cr->movementAttr, cr->slowedType,
            cr->resists, cr->ranged, cr->leavestile,
            CF_SYM(cr->rangedhittile),
            CF_SYM(cr->rangedmisstile),
            CF_SYM(cr->camouflageTile),
            CF_SYM(cr->worldrangedtile) );
}

void dumpTileRule(const Config* cfg, TileRule* rule) {
    printf( "TileRule %s 0x%x 0x%x %d %d 0x%x 0x%x\n",
            CF_SYM(rule->name), rule->mask, rule->movementMask,
            rule->speed, rule->effect, rule->walkonDirs, rule->walkoffDirs );
}

void dumpMap(const Config* cfg, Map* map) {
    printf( "Map %d %d %s %d,%d,%d,%d %d,%d,%d,%d\n",
            map->id, map->type, CF_STR(map->fname),
            map->border_behavior, map->width, map->height, map->levels,
            map->chunk_width, map->chunk_height, map->flags, map->music );

    if (map->type == Map::CITY) {
        City* city = (City*) map;
        printf( "    city \"%s\" \"%s\" %s [",
                CF_STR(city->name),
                CF_STR(city->tlk_fname),
                CF_SYM(city->cityType) );

        std::vector<PersonRole>::const_iterator it;
        foreach (it, city->personroles)
            printf("%d,%d ", (*it).role, (*it).id);
        printf("]\n");
    }
    else if (map->type == Map::DUNGEON) {
        Dungeon* dung = (Dungeon*) map;
        printf( "    dungeon %d \"%s\"\n", dung->n_rooms, CF_STR(dung->name));
    }
    else if (map->type == Map::SHRINE) {
        Shrine* shrine = (Shrine*) map;
        printf( "    shrine %d %s\n", shrine->virtue, CF_SYM(shrine->mantra));
    }

    if (! map->portals.empty()) {
        PortalList::const_iterator it;
        printf("  (\n");
        foreach (it, map->portals) {
            Portal* p = *it;
            PortalDestination* pd = p->retroActiveDest;
            if (pd) {
                printf("    ra-dest [%d,%d,%d %d]",
                       pd->coords.x, pd->coords.y, pd->coords.z, pd->mapid);
            }
            printf("    %d,%d,%d %d,%d,%d 0x%x 0x%x %d %d,%d\n",
                    p->coords.x, p->coords.y, p->coords.z,
                    p->start.x, p->start.y, p->start.z,
                    p->trigger_action, p->portalTransportRequisites,
                    p->destid, (int) p->saveLocation, (int) p->exitPortal);
        }
        printf("  )\n");
    }

    std::map<Symbol, Coords>::const_iterator it = map->labels.begin();
    if (it != map->labels.end()) {
        printf("    (");
        for (; it != map->labels.end(); ++it) {
            Coords pos = it->second;
            printf("%s %d,%d,%d ", CF_SYM(it->first), pos.x, pos.y, pos.z);
        }
        printf(")\n");
    }
}
#endif
