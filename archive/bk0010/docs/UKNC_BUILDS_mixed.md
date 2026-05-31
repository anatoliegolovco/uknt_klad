# УКНЦ (МС-0511) builds of КЛАД

Two distinct versions found and extracted.

---

## Version 1 — Баранов 1987 (original УКНЦ port)

| Field | Value |
|-------|-------|
| File | `assets/uknc/KLAD_1987_Baranov.SAV` |
| Size | 17 408 bytes (34 RT-11 blocks) |
| Source disk | `fodos_games.dsk` (FODOS filesystem, from Titus collection) |
| Source URL | `hobot.pdp-11.ru/ukdwk_archive/ukncbtlwebcomplekt/FODOS_GAMES/disk_10_fix.dsk` |

**Identification strings (KOI8-R at runtime offsets):**
- `"Николаев 1987"` @ byte 2532
- `"Баранов"` @ byte 2546
- `"Д.Г."` @ byte 2554

**Game intro text (KOI8-R, consecutive):**
```
"КЛАД"
"Игра заключается в том,чтобы"
"пройти все лабиринты,собирая"
"все клады и избегая встреч"
"с зелеными человечками,стараться"
"не упасть в воду и набрать"
"максимальное число очков"
"Управлять движением красного"
"человечка можно при помощи клавиш"
"Стрельба влево - <Q>"
"Стрельба вправо- <S>"
"Выберите скорость,нажав любую"
"из клавиш:1,2,3,4(1-max,4-min)"
```

**Mechanics notes from binary:**
- Speed selector: keys 1–4 (1=max, 4=min)
- Shoot left: Q / Shoot right: S
- HUD: `"Счет 0 Попытки 5"` (score + lives)
- No level names visible (levels probably embedded as tile data)

---

## Version 2 — Crocodile Software 1991

| Field | Value |
|-------|-------|
| File | `assets/uknc/MKLAD_1991_Crocodile.GAM` |
| Size | 19 968 bytes (39 RT-11 blocks) |
| Source disk | `newgames.dsk` (RT-11 SJ, DWK_QUEST 2019 collection) |
| Source URL | `hobot.pdp-11.ru/ukdwk_archive/ukncbtlwebcomplekt/UKNCgames_NEW/newgames.dsk` |

**Identification string (ASCII):**
- `"@ 1991 CROCODILE SOFTWARE"` @ byte 8552

**Level names (transliterated Russian, 18 levels):**
```
podzemelxe  (подземелье)     zmea       (змея)
nachalo     (начало)         uhwati     (ухвати)
lestnica    (лестница)       ba{ni^bliznecy (башни-близнецы)
koi^8       (КОИ-8)          petli      (петли)
komnaty     (комнаты)        AMIDA
pqtaq ba{nq (пятая башня)    w ozere    (в озере)
mogila      (могила)         piramida^1-4 (пирамиды 1–4)
izwrat      (извращение)     lowu{ki    (ловушки)
rezervuar   (резервуар)
```

**HUD strings:** `"o~ki:"` (очки/score), `"urowenx:"` (уровень/level)

**Win/game-over strings:**
```
"molodec!"     (молодец = well done)
"priz: @@00"   (prize)
"sledu`}ij:"   (следующий = next)
"^ uda~i ^"    (удачи = good luck)
"konec igry"   (конец игры = game over)
```

---

## Relationship to the BK-0010 builds

The УКНЦ (МС-0511) is a different machine from the БК-0010:
- BK-0010: linear framebuffer at `040000`, direct CPU access
- УКНЦ: planar video, driven by peripheral CPU (PPU); no direct CPU framebuffer

The two lineages are parallel ports by the same authors:

| Platform | 1987 original | 1991 Crocodile |
|----------|--------------|----------------|
| БК-0010 | `KLAD3.BIN` | `KLAD.BIN` (packed) |
| УКНЦ     | `KLAD_1987_Baranov.SAV` ← **this** | `MKLAD_1991_Crocodile.GAM` ← **this** |

The "М" prefix in MKLAD likely stands for "МС-0511" (the УКНЦ's official model number).

---

## How to run (emulator)

Use **UKNCBTL** (UKНЦ Back To Life) emulator:
- Load `fodos_games.dsk` or `newgames.dsk` as disk 0
- Boot RT-11, then: `RUN KLAD` or `RUN MKLAD`

The emulator is available at `hobot.pdp-11.ru/ukdwk_archive/ukncbtlwebcomplekt/ukncbtlwebcomplekt/`.

---

## Parser used to extract

Custom Python RT-11 reader (`/tmp/scan_rt11.py` in session).  
Permanent file status codes encountered:
- `0x8400` — newgames.dsk / fodos_games.dsk format
- `0x0400` — NSK-xxx educational disks format

Files extracted by computing cumulative start block from directory + copying `size × 512` bytes.
