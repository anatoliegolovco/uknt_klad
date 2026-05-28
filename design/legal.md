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

## Current status: personal, non-commercial (originals tracked)

This project is a **personal hobby** — non-commercial, not for wide
distribution. Personal reverse engineering and preservation of long-abandoned
software is the accepted-practice case the project's own research identified
("studiu personal = uz acceptabil în practică"). On that basis the original
material (binaries, extracted data, disassembly) **is tracked** in this repo at
the owner's request (decision D7).

### A brief (mis)understanding, for the record
For one exchange the goal looked commercial (a typo). *While that was assumed*,
committing the original levels was declined — and that would have been the right
call: a product **sold** with third-party level designs is infringement and
invites takedowns/claims (Kadokawa is active and litigious). The typo corrected
("**nu** planific" — *not* monetising), so that constraint does not apply here.

### The line that still matters
The "Soviet software / no private property" argument is rhetorical, not legal:
Russia has copyright law and *Rise Out* is Japanese (Kadokawa), so the works
**are** protected. What makes tracking them fine here is the **personal,
non-commercial** nature — not the works' origin. If this ever becomes something
sold or broadly published, the analysis below reapplies and shipping must shift
to original/licensed assets only.

## If this ever goes commercial — checklist before charging money

- [ ] Audit: every shipped byte of art/level/audio is original or licensed.
- [ ] No original binary, graphics, or level data in the repo or the build.
- [ ] Distinct name and branding (not "КЛАД" / not the original logo).
- [ ] Inspiration credit present; no implication of official affiliation.
- [ ] (Recommended) a lawyer's review for the target market.
