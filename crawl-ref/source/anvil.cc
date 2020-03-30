/**
 * @file
 * @brief Magical anvils.
**/

#include "AppHdr.h"

#include "anvil.h"

#include "artefact.h"
#include "env.h"
#include "enum.h"
#include "invent.h"
#include "itemprop.h"
#include "item_use.h"
#include "makeitem.h"
#include "message.h"
#include "output.h"
#include "player-equip.h"
#include "terrain.h"
#include "view.h"

//return whether the base type of the weapon sucks ass or not
static bool _basetype_sucks(item_def &item)
{
    switch(item.base_type)
    {
        case OBJ_WEAPONS:
        {
            return item.sub_type == WPN_WHIP || item.sub_type == WPN_MACE
                || item.sub_type == WPN_FLAIL || item.sub_type == WPN_RAPIER
                || item.sub_type == WPN_LONG_SWORD
                || item.sub_type == WPN_WAR_AXE || item.sub_type == WPN_HAND_AXE
                || item.sub_type == WPN_SPEAR || item.sub_type == WPN_TRIDENT
                || item.sub_type == WPN_HALBERD || item.sub_type == WPN_SHORTBOW
                || item.sub_type == WPN_DAGGER;
        }
        default:
            return false;
    }
}

/*try to improve the weapon's base type, taking into account handedness
and the plyer's skill level. Return true if successful and false otherwise*/
static bool _try_improve_weapon_basetype(item_def &item)
{
    //don't even try
    if (item.base_type != OBJ_WEAPONS)
        return false;
    
    int wpn_skill = you.skill(item_attack_skill(item), 10, true, false);
    int shield_skill = you.skill(SK_SHIELDS, 10, true, false);
    bool formicid = you.species == SP_FORMICID;
    size_type size = you.body_size(PSIZE_TORSO, true);
    
    
    switch(item.sub_type)
    {
        case WPN_WHIP:
        case WPN_MACE:
            item.sub_type = WPN_FLAIL;
            return true;
        case WPN_RAPIER:
            item.sub_type = WPN_LONG_SWORD;
            return true;
        case WPN_LONG_SWORD:
            item.sub_type = WPN_SCIMITAR;
            return true;
        case WPN_SHORTBOW:
            item.sub_type = coinflip() ? WPN_ARBALEST : WPN_LONGBOW;
            return true;
        case WPN_HAND_AXE:
            item.sub_type = WPN_WAR_AXE;
            return true;
        case WPN_SPEAR:
            item.sub_type = WPN_TRIDENT;
            return true;
        case WPN_HALBERD:
            item.sub_type = WPN_GLAIVE;
            return true;
        case WPN_WAR_AXE:
        {
            if(x_chance_in_y(100 - shield_skill, 60) || formicid)
            {
                if(size >= SIZE_MEDIUM)
                {
                    item.sub_type = WPN_BATTLEAXE;
                    return true;
                }
                else
                {
                    item.sub_type = WPN_BROAD_AXE;
                    return true;
                }
            }
            else if (size >= SIZE_MEDIUM)
            {
                item.sub_type = WPN_BROAD_AXE;
                return true;
            }
            break;
        }
        case WPN_SCIMITAR:
        {
            if(size >= SIZE_MEDIUM || x_chance_in_y(100 - shield_skill, 60))
            {
                item.sub_type = WPN_DOUBLE_SWORD;
                return true;
            }
            else if (coinflip() || size < SIZE_MEDIUM)
            {
                item.sub_type = WPN_DEMON_BLADE;
                return true;
            }
            break;
        }
        case WPN_DOUBLE_SWORD:
        {
            if(size >= SIZE_MEDIUM && (x_chance_in_y(100 - shield_skill, 60) || formicid)
                && (wpn_skill > 200 || wpn_skill < 100))
            {
                item.sub_type = WPN_TRIPLE_SWORD;
                return true;
            }
            else
            {
                item.sub_type = WPN_DEMON_BLADE;
                return true;
            }
            break;
        }
        case WPN_QUARTERSTAFF:
        {
            if (size >= SIZE_MEDIUM)
            {
                item.sub_type = WPN_GREAT_MACE;
                return true;
            }
            else
            {
                item.sub_type = coinflip() ? WPN_MORNINGSTAR : WPN_DEMON_WHIP;
                return true;
            }
            break;
        }
        case WPN_TRIDENT:
        {
            if (one_chance_in(3) || size < SIZE_MEDIUM)
            {
                item.sub_type = WPN_DEMON_TRIDENT;
                return true;
            }
            else if (x_chance_in_y(100 - shield_skill, 60) || formicid)
            {
                item.sub_type = WPN_HALBERD;
                return true;
            }
            break;
        }
        case WPN_FLAIL:
        {
            if ((x_chance_in_y(100 - shield_skill, 60)) || formicid)
            {
                item.sub_type = WPN_QUARTERSTAFF;
                return true;
            }
            else if (one_chance_in(3))
            {
                item.sub_type = coinflip() ? WPN_MORNINGSTAR : WPN_DEMON_WHIP;
                return true;
            }
            break;
        }
        case WPN_BATTLEAXE:
        {
            if (wpn_skill > 200 || wpn_skill < 100)
            {
                item.sub_type = WPN_EXECUTIONERS_AXE;
                return true;
            }
            break;
        }
        case WPN_GLAIVE:
        {
            if (wpn_skill > 200 || wpn_skill < 100)
            {
                item.sub_type = WPN_BARDICHE;
                return true;
            }
            break;
        }
        case WPN_ARBALEST:
        case WPN_LONGBOW:
        {
            if (wpn_skill > 200 || wpn_skill < 100)
            {
                item.sub_type = WPN_TRIPLE_CROSSBOW;
                return true;
            }
            break;
        }
        default:
            return false;
    }
    return false;
}

//returns whether the anvil successfully modified an item.
static bool _perform_forge_action(item_def &item)
{
    switch(item.base_type)
    {
        case OBJ_WEAPONS:
        {
            brand_type brand = get_weapon_brand(item);
            //chance to turn the item into an artifact, or add additional artifact properties
            if(one_chance_in(is_artefact(item) ? 3 : brand != SPWPN_NORMAL ? 5 : 10))
            {
                if(anvil_modify_artp(item))
                    return true;
            }
            //chance to improve base type; continue if the base type can't be improved
            if(one_chance_in(_basetype_sucks(item) ? 3: 5))
            {
                if(_try_improve_weapon_basetype(item))
                    return true;
            }
            //chance to rebrand weapon, or guaranteed rebrand if we can't raise enchant
            if (item.plus >= MAX_WPN_ENCHANT || one_chance_in(brand == SPWPN_NORMAL ? 3 : 6))
            {
                rebrand_weapon(item);
                return true;
            }
            //if we didn't do anything else, increase the weapon's enchantment
            else
            {
                item.plus = min(item.plus + 1 + random2(3), MAX_WPN_ENCHANT);
                return true;
            }
        }
        case OBJ_ARMOUR:
        {
            int max_ench = armour_max_enchant(item);
            //chance to turn the item into an artifact or add artifact properties
            if (item.plus >= max_ench || one_chance_in(is_artefact(item) ? 3 : 5))
            {
                if(anvil_modify_artp(item))
                    return true;
            }
            //if we didn't do that, raise enchantment
            else
            {
                if(item.plus < max_ench)
                {
                    item.plus = min(item.plus + 1 + random2(3), max_ench);
                    return true;
                }
            }
        }
        case OBJ_JEWELLERY:
        {
            if(anvil_modify_artp(item))
                return true;
        }
        default:
            return false;
    }
    
    return false;
}

//use an anvil and turn it dormant
void use_anvil()
{
    ASSERT(grd(you.pos()) == DNGN_ENCHANTED_ANVIL);

    int item_slot;
    item_def itemp;
    while (true)
    {
        // If this is changed to allow more than weapon/armour, a lot of the
        // below code will need to be updated.
        item_slot = prompt_invent_item("Enchant what?", MT_INVLIST,
                                        OSEL_EQUIPPABLE, OPER_ANY,
                                        invprompt_flag::escape_only);
        if (prompt_failed(item_slot))
        {
            canned_msg(MSG_OK);
            return;
        }

        itemp = you.inv[item_slot];
        
        // If the item is an unrand, enchanting it will fail
        if (itemp.flags & ISFLAG_UNRANDART)
        {
            mpr("This artifact is too powerful to reforge.");
            return;
        }

        break;
    }

    item_def &item = itemp;
  

    mprf("You break %s on the anvil...", item.name(DESC_YOUR).c_str());
    mprf("You sense the presence of a fiery spirit.");
    flash_view_delay(UA_PLAYER, YELLOW, 300);
    if (_perform_forge_action(item))
        mprf("%s is reforged!", item.name(DESC_THE).c_str());
    else
        mprf("...but %s is put back together unchanged.", item.name(DESC_THE).c_str());

    you.inv[item_slot] = item;

    // Destroy the anvil
    mpr("The anvil dulls and falls dormant.");
    auto const pos = you.pos();
    dungeon_terrain_changed(pos, DNGN_DESTROYED_ANVIL);
    view_update_at(pos);

    calc_mp();
    redraw_screen();
    you.turn_is_over = true;
    return;
}