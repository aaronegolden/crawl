/**
 * @file
 * @brief Monster-affecting enchantment spells.
 *           Other targeted enchantments are handled in spl-zap.cc.
**/

#include "AppHdr.h"

#include "spl-monench.h"

#include "coord.h"
#include "coordit.h"
#include "env.h"
#include "message.h"
#include "mon-tentacle.h"
#include "spl-util.h"
#include "terrain.h"

int englaciate(coord_def where, int pow, actor *agent)
{
    actor *victim = actor_at(where);

    if (!victim || victim == agent)
        return 0;

    if (agent->is_monster() && mons_aligned(agent, victim))
        return 0; // don't let monsters hit friendlies

    monster* mons = victim->as_monster();

    if (victim->is_stationary() || victim->check_res_magic(pow) > 0)
    {
        if (!mons)
            canned_msg(MSG_YOU_UNAFFECTED);
        else if (!mons_is_firewood(*mons))
            simple_monster_message(*mons, " resists.");
        return 0;
    }

    int duration = (1 + random2(2 + div_rand_round(pow, 6))) * BASELINE_DELAY;

    if ((!mons && you.get_mutation_level(MUT_COLD_BLOODED))
        || (mons && mons_class_flag(mons->type, M_COLD_BLOOD)))
    {
        duration *= 2;
    }

    if (!mons)
        return slow_player(duration);

    return do_slow_monster(*mons, agent, duration);
}

spret_type cast_englaciation(int pow, bool fail)
{
    fail_check();
    mpr("You radiate an aura of cold.");
    apply_area_visible([pow] (coord_def where) {
        return englaciate(where, pow, &you);
    }, you.pos());
    return SPRET_SUCCESS;
}

//similar to closest_target_in_range, but checks some other stuff
static monster* _hibernation_target()
{
    for (distance_iterator di(you.pos(), true, true, 1); di; ++di)
    {
        monster *mon = monster_at(*di);
        if (mon
            && you.can_see(*mon)
            && you.see_cell_no_trans(mon->pos())
            && !mon->wont_attack()
            && !mons_is_firewood(*mon)
            && !mons_is_tentacle_or_tentacle_segment(mon->type)
            && !mon->is_summoned()
            &&  mon->can_hibernate())
        { 
            //valid target obtained
            return mon;
        }
    }
    
    return nullptr;
}

spret_type cast_hibernation(int pow, bool fail, bool tracer)
{
    monster *mon = _hibernation_target();
    
    if (tracer)
    {
        if (!mon)
            return SPRET_ABORT;
        else
            return SPRET_SUCCESS;
    }
    
    fail_check();
    
    if (!mon)
        canned_msg(MSG_NOTHING_HAPPENS);
    else
    {
        int res_margin = mon->check_res_magic(pow);
        if (res_margin > 0)
        {
            simple_monster_message(*mon,
                    mon->resist_margin_phrase(res_margin).c_str());
        }
        else
        {
            mprf("%s falls asleep", mon->name(DESC_THE).c_str());
            mon->put_to_sleep(&you, pow, true);
        }
    }
    return SPRET_SUCCESS;
}

static bool _valid_petrify_target(monster* mon)
{
    return mon
            && you.can_see(*mon)
            && you.see_cell_no_trans(mon->pos())
            && !mon->wont_attack()
            && !mons_is_firewood(*mon)
            && !mons_is_tentacle_or_tentacle_segment(mon->type)
            && !mon->is_summoned()
            && !mon->res_petrify();
}

static monster* _gaze_target()
{
    for (distance_iterator di(you.pos(), true, true, 1); di; ++di)
    {
        monster *mon = monster_at(*di);
        if (_valid_petrify_target(mon))
        { 
            //valid target obtained
            return mon;
        }
    }
    
    return nullptr;
}

spret_type gorgons_gaze(int pow, bool fail, bool tracer)
{
    monster *mon = _gaze_target();
    
    if (tracer)
    {
        if (!mon)
            return SPRET_ABORT;
        else
            return SPRET_SUCCESS;
    }
    
    fail_check();
    
    if (!mon)
        canned_msg(MSG_NOTHING_HAPPENS);
    else
    {
        mprf("You weave a hideous illusion!");
        int res_margin = mon->check_res_magic(pow);
        if (res_margin > 0)
        {
            simple_monster_message(*mon,
                    mon->resist_margin_phrase(res_margin).c_str());
        }
        else
        {
            if (mon->check_res_magic(pow) < 0)
                mon->fully_petrify(&you);
            else
                mon->petrify(&you);
        }
        
        // check for secondary targets
        for (adjacent_iterator ai(mon->pos()); ai; ++ai)
        {
            if (grid_distance(*ai, you.pos()) == 1)
            {
                monster* secondary = monster_at(*ai);
                if (secondary && _valid_petrify_target(secondary))
                {
                    int res = secondary->check_res_magic(pow);
                    if (res > 0)
                    {
                        simple_monster_message(*secondary,
                            secondary->resist_margin_phrase(res).c_str());
                    }
                    else
                        secondary->petrify(&you);
                }
            }
        }
    }
    
    return SPRET_SUCCESS;
}

/** Corona a monster.
 *
 *  @param mons the monster to get a backlight.
 *  @returns true if it got backlit (even if it was already).
 */
bool backlight_monster(monster* mons)
{
    const mon_enchant bklt = mons->get_ench(ENCH_CORONA);
    const mon_enchant zin_bklt = mons->get_ench(ENCH_SILVER_CORONA);
    const int lvl = bklt.degree + zin_bklt.degree;

    mons->add_ench(mon_enchant(ENCH_CORONA, 1));

    if (lvl == 0)
        simple_monster_message(*mons, " is outlined in light.");
    else if (lvl == 4)
        simple_monster_message(*mons, " glows brighter for a moment.");
    else
        simple_monster_message(*mons, " glows brighter.");

    return true;
}

bool do_slow_monster(monster& mon, const actor* agent, int dur)
{
    if (mon.check_stasis(false))
        return true;

    if (!mon.is_stationary()
        && mon.add_ench(mon_enchant(ENCH_SLOW, 0, agent, dur)))
    {
        if (!mon.paralysed() && !mon.petrified()
            && simple_monster_message(mon, " seems to slow down."))
        {
            return true;
        }
    }

    return false;
}
