# osu!top

Linux-capable beatmap recommendation thingy. Similar to [osu!Trainer](https://osu.ppy.sh/community/forums/topics/209560) and [OsuHelper](https://github.com/Tyrrrz/OsuHelper/).

**Very WIP and albeit some things have been done in terms of emscripten support to run it in the browser, 
it is not currently functional and may or may not be compilable.** 

![](https://i.imgur.com/uMzytXY.png)

## Setup

TODO

Set `$OSUTOP_HOME` to some directory. This will decide where the cache will be placed.

## How it works

1. Go through target player's top plays
1. Get users who are in the top 100 on any of those plays
1. List top plays of those users that are in the range of the target player's top plays

Algorithm is planned to be improved
