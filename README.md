Inspired by [dungeon crawl short soup](https://github.com/dcandido/crawl), [coffee-crawl](https://github.com/WilliamTheMarsman/crawl) is a  [crawl](https://github.com/crawl/crawl) variant featuring the first 10 floors of dungeon followed by one floor of zot.

The game has been rebalanced to ensure the zot floor is possible for most characters at that stage of the game.

Try it out here http://coffee-crawl.codingwhileblack.xyz/#lobby.

Also: currently there's a bug where the game crashes if '\\' is pressed.

### So what's coffee-crawl all about then?

In this crawl variant, there are ten floors of dungeon and a single floor of zot. There are no branches or portal vaults. My goal was to create a game of crawl which could be won in about 15 minutes; I didn't succeed as it currently takes me around 45 minutes to win as HoFi, but I might also not be very good at the game.

Most of my ideas came from hell-crawl, dungeon crawl short soup and unimplemented idea from Taverns' GDD and CYC boards. I have an outstanding [TODO list](https://github.com/WilliamTheMarsman/crawl/blob/coffee-crawl/TODO.md) with some additional ideas which didn't make it in.

Here are the main changes.

1. Dungeon is now 10 floors instead of 15.
1. Temple always appears on D2.
1. Zot now comes directly after dungeon and requires no runes.
1. Zot and Zot entrance enemies have been balanced for the power of mid-tier characters after 10 floors, although pure mages may struggle as monsters still have their old resistances. Monster spellbook and stat changes have been made to reduce the difficulty for all end game monsters encountered in this variant.
1. It is no longer possible to collect any runes.
1. All methods of accessing the abyss have been removed.
2. All other branches and and all portals besides baazar have been removed. I left baazar because I love shops.
3. drop restrictions have been lifted from early floors; better gear is found deeper, but it is now possible to find scrolls of acquirement, triple swords, manuals and evokables on D1.
4. Shops can now be placed on all floors, instead of only later floors.
5. The range of floors a monster can be placed on has been tightened, since there are now only 10 floors. Adders no longer have a chance of being placed on D10, for instance.
6. The orb run has been removed. The game is won once the player leaves Zot with the Orb.
7. Skill cross-training changes to make most characters like gnolls. With only 10 floors, there isn't much experience to allocate which I didn't really like. Now there are basically 5 skill groups which all cross-train each other, to allow players to be better at using what the dungeon provides. See [cross-training](./CROSS-TRAINING.md) for more details.
