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


//returns whether the anvil successfully modified an item.
static bool _perform_forge_action(item_def &item)
{
    switch(item.base_type)
    {
        case OBJ_WEAPONS:
        {
            if(one_chance_in(is_artefact(item) ? 3 : 5))
            {
                anvil_modify_artp(item);
                return true;
            }
            brand_type brand = get_weapon_brand(item);
            if (item.plus >= MAX_WPN_ENCHANT || one_chance_in(brand == SPWPN_NORMAL ? 3 : 6))
            {
                rebrand_weapon(item);
                return true;
            }
            else
            {
                item.plus = min(item.plus + 1 + random2(3), MAX_WPN_ENCHANT);
                return true;
            }
        }
        case OBJ_ARMOUR:
        {
            int max_ench = armour_max_enchant(item);
            if (item.plus >= max_ench || one_chance_in(is_artefact(item) ? 3 : 5))
            {
                anvil_modify_artp(item);
                return true;
            }
            else
            {
                item.plus = min(item.plus + 1 + random2(3), max_ench);
                return true;
            }
        }
        case OBJ_JEWELLERY:
        {
            anvil_modify_artp(item);
            return true;
        }
        default:
            return false;
    }
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

    redraw_screen();
    you.turn_is_over = true;
    return;
}