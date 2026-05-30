# КЛАД — История, рецензии и советы по прохождению

> Compiled from: gamedev.ru forum (2005–2007), LiveJournal, gradmsk.ru, old-games.ru wiki.  
> All Russian quotes are translated; originals preserved in parentheses where relevant.

---

## 1. Оригинал: Rise Out (1983, MSX)

| Field | Value |
|-------|-------|
| Title | **Rise Out** (ライズアウト) |
| Publisher | **ASCII Corporation** (株式会社アスキー) — same company that co-created the MSX standard with Microsoft |
| Platform | **MSX 1** |
| Year | **1983** |
| CPU | Zilog **Z80** @ 3.58 MHz |
| Video | **TMS9918A** — 256×192 px, 16 colours (sprites: 8×8 or 16×16, max 4 per scanline) |

Rise Out is the Japanese original from which all Soviet КЛАД versions descend. The MSX was a standardised home-computer architecture dominant in Japan in the early 1980s — the Soviet КЛАД ports are **complete rewrites** for a completely different CPU family (PDP-11), not binary conversions.

---

## 2. Soviet ports: who, when, for what machine

| Year | Author | Platform | Notes |
|------|--------|----------|-------|
| **1987** | **Баранов Д.Г.** (Nikolaev, Ukraine) | **БК-0010** | Original Soviet port — `KLAD.BIN` / `KLAD3.BIN` in this repo |
| **1987** | Баранов Д.Г. | **УКНЦ (МС-0511)** | Parallel port — `KLAD_1987_Baranov.SAV` in this repo |
| **1991** | **Crocodile Software** | УКНЦ | Repackaged port — `MKLAD_1991_Crocodile.GAM`; adds 20 named levels |
| 1988 | Баранов | БК-0010 | КЛАД-2 (sequel) |
| 1988 | Баранов | БК-0010 | КЛАД-3 |
| ~1989 | ? | БК-0010 | КЛАД-4 |

The BK-0010 and УКНЦ 1987 versions share ~83% of the binary code (confirmed by diff analysis). The CPU is the same family (К1801ВМx, PDP-11 compatible), but I/O differs: the BK-0010 has a linear framebuffer, while the УКНЦ drives video through a peripheral processor.

---

## 3. Gameplay — что это за игра (what kind of game it is)

**Жанр / Genre:** Arcade puzzle — labyrinth + platformer.  
**Ближайший западный аналог / Closest Western equivalent:** Lode Runner (1983, Broderbund) — but with key differences.

### Отличия от Lode Runner / Differences from Lode Runner

| Mechanic | Lode Runner | КЛАД |
|----------|-------------|-------|
| Destroy obstacles | Dig holes in the floor | **Shoot walls left/right** (Q = left, S = right on УКНЦ) |
| Destroyed obstacle | Permanent hole | **Regenerates** after a few seconds |
| Enemies | Fall in holes, get revived | **Infinite lives**, meet = instant death |
| Goal per level | Collect all gold, then climb to exit | **Find the KEY** among treasures, then reach the exit |
| Fall mechanic | None | Falling onto certain floor types **falls through** |
| Water/fire | None | **Water kills**, some floors are hazards |
| No ladders (ceiling bars) | Yes (ceiling bars) | **No ceiling bars** — only floor ladders |
| Save | No | No (some emulators add save states) |

From the gamedev.ru forum (user *Tolking Pet*, 2005):
> «В Кладе стрельба, в Lode Runner'е — копание ям. Геймплей разный. Рульная игра... Жалко, сэйва не было.»  
> *"In КЛАД there's shooting, in Lode Runner there's digging. Completely different gameplay. Great game... Too bad there was no save."*

---

## 4. Механика врагов / Enemy mechanics

This is the key insight that separates КЛАД from simple arcade running:

**Enemy AI (confirmed from gamedev.ru, user *lbdv*, 2005):**
> «Вражина сначала пытался совпасть координатой X и только затем координатой Y — именно поэтому игра не сваливалась в тупой экшен с беготнёй, т.к. всё можно было сначала рассчитать.»  
> *"The enemy first tries to match the X coordinate and only then the Y coordinate — this is exactly why the game didn't devolve into dumb running action, because everything could be calculated in advance."*

This makes the game a **logic puzzle** rather than a reflex game. You can plan a route knowing exactly where each enemy will move next.

**Trapping enemies:**  
Shoot two adjacent wall blocks, timed so the enemy walks between them as one regenerates.  
- An enemy **inside a single regenerated block** is NOT trapped — it walks through.  
- An enemy **between two regenerated blocks** IS trapped permanently (or until you shoot again).

From gamedev.ru (user *stopkin*, 2007, re: level 16):
> «Замуровывал обоих монстров в нижней шахматке. Сложно — но осуществимо. Минимальное расстояние до монстра — 2 лестницы и 2 кирпича, кирпичи пробивать в разное время. Пока монстр бежит через один кирпич — другой затягивается, и монстр в ловушке.»  
> *"I trapped both monsters in the lower checkerboard section. Difficult but doable. Minimum distance to monster: 2 ladders and 2 bricks. Shoot the bricks at different times — while the monster runs through one, the other seals up, and the monster is trapped."*

---

## 5. Скорость / Speed levels

The game has 4 speed settings (1 = max, 4 = min — confirmed from binary intro text).  
Keyboard: press **1, 2, 3, or 4** at the speed selection screen.

**Tip:** Difficult levels become significantly more manageable at speed 4 (slowest). The 2007 forum discussion about level 16 concluded that the user's failure was partly due to playing at speed 2 — switching to speed 4 let them complete it in under 5 minutes.

---

## 6. Уровни / Levels

### КЛАД Part 1 (1987 Барановская версия)
At least **16 levels** confirmed from player accounts (gamedev.ru forum, 2007 — user got stuck on level 16).

### MKLAD (Crocodile Software 1991, УКНЦ)
**20 named levels** — names extracted from binary (`MKLAD_1991_Crocodile.GAM`):

| # | Transliterated name | Russian | Translation |
|---|---------------------|---------|-------------|
| 1 | podzemelxe | Подземелье | Dungeon |
| 2 | zmea | Змея | Snake |
| 3 | nachalo | Начало | Beginning |
| 4 | uhwati | Ухвати | Catch/Grab it |
| 5 | lestnica | Лестница | Ladder/Staircase |
| 6 | ba{ni bliznecy | Башни-близнецы | Twin Towers |
| 7 | koi-8 | КОИ-8 | KOI-8 (the Russian character encoding) |
| 8 | petli | Петли | Loops |
| 9 | komnaty | Комнаты | Rooms |
| 10 | AMIDA | AMIDA | Amida (Japanese lottery game — indicates authorship knowledge) |
| 11 | pqtaq ba{nq | Пятая башня | Fifth Tower |
| 12 | w ozere | В озере | In the Lake |
| 13 | mogila | Могила | Grave |
| 14 | piramida-2 | Пирамида №2 | Pyramid 2 |
| 15 | piramida-3 | Пирамида №3 | Pyramid 3 |
| 16 | izwrat | Извращение | Perversion / The Twisted One |
| 17 | piramida-1 | Пирамида №1 | Pyramid 1 |
| 18 | piramida-4 | Пирамида №4 | Pyramid 4 |
| 19 | lowu{ki | Ловушки | Traps |
| 20 | rezervuar | Резервуар | Reservoir |

Level names are stored as transliterated ASCII strings (not Cyrillic) inside the binary, surrounded by `&` and `$` delimiters.

---

## 7. Level 16 — documented solution

This is the only level with a documented community solution (gamedev.ru, 2007).

**Setup:** The key is in the upper part of the map. A monster appears at the top that blocks the exit path.  
**Solution:**
1. Find and collect the key first.
2. Trap **both** monsters in the lower checkerboard section using the double-wall technique (shoot two adjacent walls, time them so the monster walks between them as one regenerates).
3. With monsters trapped, freely collect remaining treasures and exit.
4. If the timing is too difficult: switch to **speed 4** (slowest). The poster solved it in under 5 minutes after switching.

---

## 8. Рецензии и впечатления / Reviews and impressions

### «Умная игра» / "An intelligent game"
*gamedev.ru, user lbdv, 2005:*
> «Хотя можно назвать клоном Lode Runner'а, мне кажется более "умная" игра.»  
> *"Although it can be called a Lode Runner clone, I think it's a more 'intelligent' game."*

### Наглядный пример хорошего тупого AI / A good example of beneficial dumb AI
*gamedev.ru, user lbdv, 2005:*
> «Наглядный пример, как тупой AI врагов может идти на пользу геймплею.»  
> *"A clear example of how dumb enemy AI can benefit gameplay."*

### Каждый уровень = алгоритм / Every level = an algorithm
*gamedev.ru, user Tolking Pet, 2005:*
> «Почти на каждом нужно было думать, и вырабатывать алгоритм прохождения.»  
> *"Almost every level required thinking and developing an algorithm to pass it."*

### Советские игровые залы / Soviet gaming arcades
*LiveJournal, user jjolyk, 2011:*
> «Были конторы где за 50 советских копеек на 10 (или 20?) минут можно было поиграть.»  
> *"There were places where for 50 Soviet kopecks you could play for 10 (or 20?) minutes."*  
> Last played in 1989. Re-discovered via emulator in 2011 — still enjoyable.

### Легенда / A legend
*gamedev.ru, user ShTiRLiC, 2007:*
> «Ты офигел что ли, на легенду гнать? Такие игры просто не обсуждаются, они были и остаются такими, какие есть, ни отнять, ни прибавить. Еще тетрис поругай, за то, что там только 5 фигурок — не гуд!»  
> *"Are you crazy, criticising a legend? Such games simply aren't debated — they were and remain what they are, you can't add or subtract from them. Go ahead and criticise Tetris too, for only having 5 pieces — not good!"*  
> (Said to a gamer from the Nintendo/Dendy generation who initially dismissed the game's 4-colour graphics — and then started playing and took it back.)

### Полное прохождение / Full playthrough
A **2-hour 25-minute** full playthrough of КЛАД Part 1 exists on YouTube (as of 2024):  
*"Напряг мозга и хардкор — «Клад» для БК-0010. Прохождение, FULL PLAY."*  
Description: *"One of the very hard brain games. You need to find a key among the treasures and escape from the labyrinth. Strange little guys with infinite lives will constantly interfere — meeting them leads to instant death and starting the level over!"*

A **1-hour 12-minute** walkthrough of "дополнительные миссии" (additional missions = КЛАД parts 2–4) also exists on VK/Rutube.

---

## 9. HUD / Интерфейс

From binary analysis of `KLAD_1987_Baranov.SAV`:
```
Счет      0     Попытки     5
```
- **Счет** = Score (starts at 0)
- **Попытки** = Lives/Attempts (starts at 5)

From `MKLAD_1991_Crocodile.GAM` (transliterated):
```
o~ki: 00        urowenx: [level name]
```
- **Очки** = Score
- **Уровень** = Level (shows the level name)

Win screen strings (Crocodile 1991):
- `molodec!` = Молодец! = "Well done!"
- `priz: @@00` = Приз = Prize/Bonus
- `sledu{ij:` = Следующий = "Next:"
- `^ uda~i ^` = Удачи = "Good luck"
- `konec igry` = Конец игры = "Game over"

---

## 10. Советы по прохождению / General tips

1. **Choose speed 4** (slowest) until you understand each level's layout. Speed 1 is for veterans only.
2. **Enemy path is deterministic**: the enemy always tries to match your X position first, then Y. You can walk to a specific column to lure an enemy into a trap.
3. **Wall shots time out**: a destroyed wall regenerates in a fixed number of ticks. Learn the timing — it's the same for every wall in every level.
4. **Trap before collecting**: on levels with aggressive monster placement, trap the monsters first, then collect treasures at leisure.
5. **The key is hidden**: the key is visually indistinguishable from a treasure chest. Collect everything — the exit won't open until you have the key.
6. **Fall physics**: not all floors are solid. Certain tile types cause the player to fall through — use this as a shortcut, or avoid it when it would mean falling into water.
7. **Level skip (forgotten)**: a key combination reportedly existed to advance to the next level without completing the current one (mentioned in a 2012 comment, unconfirmed). Emulator save states serve the same purpose.

---

## Sources

| Source | Type | URL |
|--------|------|-----|
| gamedev.ru forum | Community discussion, 2005–2007 | `gamedev.ru/flame/forum/?id=84` |
| old-games.ru wiki | Encyclopedia | `old-games.ru/wiki/Клад` |
| jjolyk LiveJournal | Personal nostalgia post, 2011 | `jjolyk.livejournal.com/33072.html` |
| gradmsk.ru / YouTube | Full 2h25m playthrough, 2024 | Video ID: `jUv3u2txfBc` |
| VK / Rutube | 1h12m additional missions walkthrough | `rutube.ru/video/529a22a8f6cee7bbb1fa216af3efbe7e/` |
| generation-msx.nl | Rise Out original MSX page | `generation-msx.nl/software/ascii-corporation/rise-out/353` |
| MKLAD_1991_Crocodile.GAM | Binary analysis (this repo) | `assets/original/extracted/uknc/` |
| KLAD_1987_Baranov.SAV | Binary analysis (this repo) | `assets/original/extracted/uknc/` |
