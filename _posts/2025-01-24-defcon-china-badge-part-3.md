---
layout: post
title:  "Defcon China Badge - Part 3"
date:   2025-01-24
---
Okay now a little side journey I took during this. In part two you might have noticed my snazzy badge back. This was a quick 3D model I made to protect the badge since it's getting a little beat up during #badgelife sessions bumping into other badges and I'm using the USB connector a lot. Feeling a bit concerned that I'd rip it off eventually IMO this badge needs more backbone. I'm briefly sharing my process so anyone with a printer can get some super simple design going and gradually improve them with practice.

There's a lot of better modeling tools but I've got a process that works for me in PrusaSlicer. First off I attempted to use the badge design PDF from the Defcon media server (inside the [badge files rar archive](https://media.defcon.org/DEF%20CON%20China%201/DEF%20CON%20China%201%20badge/)). Importing it into [Inkscape](https://inkscape.org), that badge outline didn't end up giving me a nice clean vector, instead it ended up being 17k tiny little unconnected line segments.

![17k was way too many node segments](/images/defcon_china_badge_inkscape_1.png)
# 💥 #

Attempting to combine them and simplify on the path didn't work for me so after fighting with it a while I just used the [badge back JPG](https://media.defcon.org/DEF%20CON%20China%201/DEF%20CON%20China%201%20badge/DEF%20CON%20China%201%20Flexi%20Badge%20-%20back%202.jpg) and did a trace bitmap.  Lame but it worked. I ended up with a nice clean line with only 127 nodes.


<img src="/images/defcon_china_traced_outline.png" width="auto" width="100%" alt="Inkscape screen showing a clean simple outline SVG file"/>


From there I turned it into the rough outline shape in Tinkercad and that was the base of my model in PrusaSlicer.  

<img src="/images/defcon_china_tinkercad.png" width="auto" width="100%" alt="Tinkercad screen showing traced inner outline shape having been extruded for height, giving a flat back shape"/> 

A quick tip on making an inner and outer perimeter, you can't simply copy a path and increase it by 5% to make a wrapped shape. The outer ring is actually a different shape and if you only enlarge the inner shape you'll end up with gaps.  

<img src="/images/defcon_china_badge_tinkercad.png" width="auto" width="100%" alt="Tinkercad screen showing traced outer outline shape having been extruded adding a taller edge protection border"/>

The trick is using Inkscape and the Path > Outset function. This truly gives you a perfect fit outer track that you can use for a surrounding wall as I've done for the badge edge.

Back to PrusaSlicer, with a working initial shape, I then just right click on an object and use Add Part > Box slowly adding on clips and hole backing as I need it, modifying with negative modifers such as the hole and USB punch outs. Also I name the objects along the side so it's easy to find the parts I want to adjust.

<img src="/images/defcon_china_badge_slicer_in_progress.png" width="auto" width="100%" alt="PrusaSlicer screen showing unsliced model with negative modifiers and a clean hierarchy list of parts with each piece named for clarity"/>

I then export the plate to STL, and import it to a new project.  The reason is, the slicer will attempt to print each object even if they're overlapping, you can see the difference in these screenshots.  Notice double the external perimiter print time when it's discrete objects. Also see the G-code path error. 

<img src="/images/defcon_china_badge_slicer_conflicts.png" width="auto" width="100%" alt="PrusaSlicer screen showing many overlapping parts and error messages"/>
<img src="/images/defcon_china_badge_slicer_no_conflicts.png" width="auto" width="100%" alt="PrusaSlicer screen showing harmonious parts and no error messages"/>

Having overlapping parts in the print path causes cascading collusions where plastic already exists and neither part is compensating for the quickly growing blob that eventually destroys the whole print. So the export to clean it up step is necessary. 

Oh.... uhhh.... prototypes.  Tweak as you go, sooner or later it's done.
<img src="/images/defcon_china_badge_prototypes.png" width="auto" width="100%" alt="A dozen or so gradually changing badge back attempts"/>

<small>"Why didn't you just do all the modeling in Tinkercad?"     ...       `:|`     ..      That's a very good point.</small>

Personally I gravitate to open source solutions even when they're sub-optimal. In future projects I'll be trying to remove any dependency on closed source steps. 


I also added a lobster clasp line because the lanyard clip for this badge is pretty strong and I didn't want to bother getting it to fit in the hole, it's already almost ripping the badge as is. Alright....  black or red, which looks better with this badge?

<img src="/images/defcon_china_badge_lobsters.png" width="auto" width="100%" alt="Badge with protector and two lobster lines holding the weight off the delicate plastic."/>

(How I am I so wise in the ways of lobster lanyards?)
<img src="https://shop.defcon.org/cdn/shop/files/video_slide_badge_psa_2048x.jpg" width="auto" width="100%" alt="Official Defcon shop page showing 2 different lobster clip lanyard styles due to length differences"/>[Source](https://shop.defcon.org/products/dc32-badge)

I ended up with short lanyards for the DC32 badge I got, so looking around I found some longer replacements and picked up 2 packs for extremely cheap.

[Amazon Red ones](https://www.amazon.com/gp/product/B0BC3W34TG/)

[Amazon Black ones](https://www.amazon.com/gp/product/B0BZPSZ29N/)

So "lanyard lariat lobster clasps" got me finding the right stuff, and you want the 70mm end to end length for the DC32 badge to comfortably loop the loose way. Guess I have a lifetime supply now, I'll add them to any badges I have with iffy connections or where the clip is rubbing into the PCB.

[Thingiverse model](https://www.thingiverse.com/thing:6936572)
