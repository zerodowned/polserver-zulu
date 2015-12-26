This project is an unofficial branch of **Penultima Ultima Online** server emulator.

The original **POL** server can be downloaded at [www.polserver.com](http://www.polserver.com)

This version is a fork that was customized for **[Zuluhotel.net](http://Zuluhotel.net)** Shard.

Based of Stable beta POL099 (2009) (SVN [Revision 180](https://code.google.com/p/polserver-zulu/source/detail?r=180)) with many of the new features from official SVN releases are back-ported.

  * All of the known memory leaks closed.
  * Backported **XML** support.
  * Backported **MySQL** support with **utf-8** encoding.
  * Backported diagonal move pathfinding issues.
  * Fixed incorrect graphic while mounted and shape-shifted issues.
  * Some of the new array API added.
  * Unicode GUMPS added.
  * Some unicode functions now accept strings instead of arrays.
  * **CAscZ** and **CChrz** correctly decode and encode unicode strings.

see **core-backport.txt** for full list.

> The motivation for this fork is that I needed a stable core to run a busy shard
> where players are not so tolerant to server crashes and in-game glitches.

> While I totally support core POL team in their goal to explore new
> horizons of programming, try new approaches and innovate. These
> new things always need sufficient amount of QA which is a very rare
> commodity. Thus I took the most stable and relatively fresh core as a
> foundation while back porting many of the changes POL team had created
> over the last years.

> I am not affiliated with POL developers and in fact my contribution to the core
> is quite tiny.
> Therefore all credit goes to **[PenUltima Online Server Project](http://www.polserver.com)**
> You may use this software for whatever purposes original POL license permits.