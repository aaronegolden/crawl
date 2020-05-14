/**
 * @file
 * @brief Functions with decks of cards.
**/

#ifndef DECKS_H
#define DECKS_H

#include "enum.h"

#define NEMELEX_TRIPLE_DRAW_KEY "nemelex_triple_draw"
#define NEMELEX_STACK_KEY       deck_name(DECK_STACK)

/// The minimum number of cards a deck starts with, when generated normally.
const int MIN_GIFT_CARDS = 4;
/// The maximum number of cards a deck starts with, when generated normally.
const int MAX_GIFT_CARDS = 9;

enum deck_type
{
    DECK_OF_ESCAPE,
    FIRST_PLAYER_DECK = DECK_OF_ESCAPE,
    DECK_OF_DESTRUCTION,
    DECK_OF_SUMMONING,
    LAST_PLAYER_DECK = DECK_OF_SUMMONING,
    DECK_OF_PUNISHMENT,
    DECK_STACK,
    NUM_DECKS
};

enum card_type
{
    CARD_VELOCITY,            // remove slow, alter others' speeds

    CARD_TOMB,                // a ring of rock walls
    CARD_EXILE,               // banish others, maybe self
    CARD_SHAFT,               // under the user, maybe others

    CARD_VITRIOL,             // acid damage
    CARD_CLOUD,               // encage enemies in rings of clouds
    CARD_STORM,               // wind and rain
    CARD_PAIN,                // necromancy, manipulating life itself
    CARD_TORMENT,             // symbol of
    CARD_ORB,                 // pure bursts of energy

    CARD_ELIXIR,              // restoration of hp and mp
    CARD_SUMMON_DEMON,        // dual demons
    CARD_SUMMON_WEAPON,       // a dance partner
    CARD_SUMMON_FLYING,       // swarms from the swamp
    CARD_WILD_MAGIC,          // miscasts for everybody
    CARD_STAIRS,              // moves stairs around
    CARD_WRATH,               // random godly wrath
    CARD_WRAITH,              // drain XP
    CARD_XOM,                 // 's attention turns to you
    CARD_FAMINE,              // starving
    CARD_CURSE,               // curse your items
    CARD_SWINE,               // *oink*

    CARD_ILLUSION,            // a copy of the player
    CARD_DEGEN,               // polymorph hostiles down hd, malmutate
    CARD_ELEMENTS,            // primal animals of the elements
    CARD_RANGERS,             // sharpshooting

    NUM_CARDS
};

enum card_flags_type
{
                      //1 << 0
    CFLAG_SEEN       = (1 << 1),
                      //1 << 2
    CFLAG_PUNISHMENT = (1 << 3),
    CFLAG_DEALT      = (1 << 4),
};

const char* card_name(card_type card);
card_type name_to_card(string name);
const string deck_contents(deck_type deck);
int deck_cards(deck_type deck);
const string deck_flavour(deck_type deck);
deck_type ability_deck(ability_type abil);

bool gift_cards();
void reset_cards();
string deck_summary();
bool deck_draw(deck_type deck);
bool deck_triple_draw();
bool deck_deal();
string which_decks(card_type card);
bool deck_stack();

bool draw_three();
bool stack_five(int to_stack);

void draw_from_deck_of_punishment(bool deal = false);
string deck_status(deck_type deck);
string deck_name(deck_type deck);
string deck_description(deck_type deck);
const string stack_top();
const string stack_contents();
void card_effect(card_type which_card, bool dealt = false,
        bool punishment = false,
        bool tell_card = true);

#endif
