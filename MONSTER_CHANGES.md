# Monster changes

To make Zot easier, I am going to change the following monsters to be weaker.

| Done | Monster               | Monster Enum               | Proposesd Comparable  |
| ---- | --------------------- | -------------------------- | --------------------- |
| v2   | MOTH_OF_WRATH         | MONS_MOTH_OF_WRATH         | MONS_VAMPIRE_MOSQUITO |
| v3   | BLACK_DRACONIAN       | MONS_BLACK_DRACONIAN       | MONS_TENGU_WARRIOR    |
| v3   | YELLOW_DRACONIAN      | MONS_YELLOW_DRACONIAN      | MONS_ORC_WARRIOR      |
| v3   | GREEN_DRACONIAN       | MONS_GREEN_DRACONIAN       | MONS_ORC_WARRIOR      |
| v3   | PURPLE_DRACONIAN      | MONS_PURPLE_DRACONIAN      | MONS_ORC_WARRIOR      |
| v3   | RED_DRACONIAN         | MONS_RED_DRACONIAN         | MONS_ORC_WARRIOR      |
| v3   | WHITE_DRACONIAN       | MONS_WHITE_DRACONIAN       | MONS_ORC_WARRIOR      |
| v3   | DRACONIAN_STORMCALLER | MONS_DRACONIAN_STORMCALLER | MONS_DEATH_KNIGHT     |
| v3   | DRACONIAN_MONK        | MONS_DRACONIAN_MONK        | MONS_ORC_WARRIOR      |
| v3   | DRACONIAN_SHIFTER     | MONS_DRACONIAN_SHIFTER     | MONS_ORC_SORCERER     |
| v3   | DRACONIAN_ANNIHILATOR | MONS_DRACONIAN_ANNIHILATOR | MONS_DEEP_ELF_MAGE    |
| v3   | DRACONIAN_KNIGHT      | MONS_DRACONIAN_KNIGHT      | MONS_ORC_KNIGHT       |
| v3   | DRACONIAN_SCORCHER    | MONS_DRACONIAN_SCORCHER    | MONS_ORC_SORCERER     |
| v2   | KILLER_KLOWN          | MONS_KILLER_KLOWN          | MONS_RAKSHASA         |
| v2   | DEATH_COB             | MONS_DEATH_COB             | MONS_HUNGRY_GHOST     |
| v3   | CURSE_TOE             | MONS_CURSE_TOE             | MONS_EYE_OF_DRAINING  |
| v2   | TENTACLED_MONSTROSITY | MONS_TENTACLED_MONSTROSITY | MONS_UGLY_THING       |
| v2   | ELECTRIC_GOLEM        | MONS_ELECTRIC_GOLEM        | MONS_OGRE_MAGE        |
| v2   | ORB_OF_FIRE           | MONS_ORB_OF_FIRE           | MONS_EFREET           |
| v2   | QUICKSILVER_DRAGON    | MONS_QUICKSILVER_DRAGON    | MONS_SWAMP_DRAGON     |
| v2   | SHADOW_DRAGON         | MONS_SHADOW_DRAGON         | MONS_SWAMP_DRAGON     |
| v2   | STORM_DRAGON          | MONS_STORM_DRAGON          | MONS_SWAMP_DRAGON     |
| v2   | GOLDEN_DRAGON         | MONS_GOLDEN_DRAGON         | MONS_SWAMP_DRAGON     |
| v3   | ORB_GUARDIAN          | MONS_ORB_GUARDIAN          | None                  |

## v1 balancing results

After completing v1 balancing, I tested out zot in wizard mode. I ran through the dungeon and to zot with 27 in all skills, but no other adjustments. I picked up items as I went to get a sense of what an average run might have after running the previous floors.

Then, I entered zot and fought the enemies there. They were still much stronger than what I was prepared for. I also saw some enemies as part of vaults which were not rebalanced, so I will need to revisit that.

I set all my skills to 14, which is still likely much better than what people would have at that time, and tried fighting some enemies. Overall, everything was still too strong. Orb of Fire killed me almost a dozen times, and absorbed at least fifty hits before going down. Damage-wise, things might be okay, but enemies still feel way too bulky. I will divide AC, EV and HP of the above enemies by two and see how that feels. This will bring most enemies closer to the durability of the average orc warrior. I will revisit damage after the durability issues have been revisited.

## v2 balancing actions

Cut hp in half for most Zot monsters. Brought AC and EV down in some cases.

## v2 balancing results

- [x] Draconian AC and EV may still be slightly too high, reduce AC by 2 and EV by 2.
- [x] Curse toe AC still too high at 18. Bring down to 11.
- [x] Able to beat Orb of Fire in 1v1 at all skills at 14 with a shield of fire resist. Still feels dangerous but not impossibe. Being seen by one at a distance is maybe too dangerous, but no change at the moment.
- [x] Orb guardians probably a little too weak defensively at 300 HP, 4 AC 4 EV. Will bump HP up to 375. On the other hand, their offense is pretty have at 45 per hit. I will drop this down to 30.
- [x] I messed up when making the upgraded draconians. Currently they have an average of 18 AC and 20 EV. Will fix for v3.

Not sure about killer clowns. Will rebalance the above first.

Part of the difficulty is in having items unidentified. I will tune with items idenitified.

## v3 balacing results

Still maybe a bit too hard, but I'm happy enough with it for now.