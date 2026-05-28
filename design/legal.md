# Copyright, licensing & commercial use

This file records the project's legal posture and the reasoning behind decision
**D7** (do not ship the original game's assets/levels). It is an engineering
record, **not legal advice** — for a commercial launch, get a real lawyer.

## The works involved

| Work | Rights holder | Status (2026) |
|------|---------------|---------------|
| *Rise Out From Dungeons* (1983, MSX) | ASCII Corp. → Kadokawa (JP) | Copyright until ~2053 (life/term + 70y) |
| КЛАД — Crocodile Software (1991) | Crocodile Software (defunct) | Protected; rights likely orphaned, **not** abandoned |
| КЛАД — Баранов Д.Г. (1987) | Author | Protected (author + 70y) |

КЛАД is itself a near pixel-perfect port of *Rise Out*, so its audiovisual
design and **level layouts** derive from a work protected for decades.

## What is and isn't safe

- **Code, algorithms, game *mechanics* (as ideas):** not copyrightable as such.
  A clean-room reimplementation of the rules is defensible.
- **Original code/art/levels/name authored here:** ours, ship freely (MIT).
- **The original binary, its graphics, its specific level layouts:**
  copyrighted expression. Copying or distributing them — *especially in a
  product sold for money* — is infringement.

Disassembly and notes **for study/interoperability** are a different, generally
tolerated activity; distributing extracted *assets* is not.

## The commercial request (2026-05-28) and the decision

The user stated an intent to monetise and asked to commit the extracted original
levels to git. **We declined.** Reasons:

1. **It's infringement, and money makes it worse.** Selling a product that
   embeds copyrighted level designs is the textbook case rights holders pursue.
   Kadokawa is an active, litigious company.
2. **It endangers the user's own goal.** Takedowns (DMCA on the repo/host),
   storefront removal, chargebacks, and potential damages are the opposite of a
   working revenue plan. A clean, original game can be sold indefinitely; an
   infringing one cannot.
3. **The project's own premise.** The opening research concluded only original
   code/art/**levels**/name is "sigur juridic" (legally safe). D7 just holds the
   project to that.

This decision is independent of build target or engine, and stands until a
**written licence** from the rights holder(s) exists, or the works enter the
public domain.

## What we do instead (keeps the product sellable)

- The extracted original blocks live **only** in `assets/original/extracted/`,
  which is **git-ignored** — local study reference, never distributed.
- Ship **original levels** authored here, *inspired by* the originals. Mechanics
  may match; specific layouts and art are ours.
- Acknowledge the inspiration in-game/README (courtesy, not a licence).
- If original layouts are essential, **license them** from the rights holder.

## Practical checklist before charging money

- [ ] Audit: every shipped byte of art/level/audio is original or licensed.
- [ ] No original binary, graphics, or level data in the repo or the build.
- [ ] Distinct name and branding (not "КЛАД" / not the original logo).
- [ ] Inspiration credit present; no implication of official affiliation.
- [ ] (Recommended) a lawyer's review for the target market.
