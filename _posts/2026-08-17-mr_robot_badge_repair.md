---
layout: post
title:  "Mr. Robot Badge Repair"
date:   2026-08-17
image: /images/mr_robot_v1_00.png
---

I happened to see an original v1 [Mr. Robot badge](https://web.archive.org/web/20180720102107/http://www.mrrobotbadge.tv/) go up for sale listed clearly **defective**. Since I already have a v2 and was [familiar with it's design](https://github.com/kscottz/MrRobotStarterPack), I figured I'd give it a shot and see if I could get it working.  

From the [hackaday page](https://hackaday.io/project/18508-mr-robot-badge) the full schematic and firmware BIN file were posted. I figured worst case if there's damage on the actual PCB traces I could bodge the connections and replace any defective components. 

This finally gave me a good excuse to install KiCad and review board files.  
<img src="/images/mr_robot_v1-01.png" width="auto" width="100%" alt="PCB Editor screenshot showing invisible traces and such"/>

With the electronic schematic showing the logical design, and the board files showing the physical layout I definitely have no excuse not to be able to get this working. 😬 

Starting out by examining the board, it would not power on. Though a brief blink of the blue LED on the ESP miniboard showed that some level of power was getting through. I used a multimeter on the VCC and GND pins of the package and saw a weird 1.5v supply.  That lead me to testing the battery packs and the power convertor chip. 

<img src="/images/mr_robot_v1-02.png"  width="auto" width="100%" alt="Close up of battery holder and power adaptor chip"/>

After finding it's datasheet and identifying pin 1 by the dot you can see there, I verified it was correctly outputting 3.3v and not 1.5v sooo.... what's the deal? I attempted to power it from the UART pins with a FTDI board set to 3.3v. Though the blue pin lit up, nothing was seen on the serial TX pin at all.  Reviewing the code to the v2 board, I noticed they didn't set up serial output anywhere, so likely the v1 board behaves the same way and is very bare bones with no output while running the badge firmware. So looking for serial output with the screen command isn't the right approach. 

We need to use the ESP tools to test this one. The board however, couldn't even be identified by esptool. Since I had a [flash process example](https://github.com/kscottz/MrRobotStarterPack#flashing-firmware) from the v2 Starter Pack page, and the BIN for v1 from hackaday I figured maybe I just needed to flash what was a failed write.  But after several attempts the board just wasn't communicating at all, but some random bytes could get read, no data passing CRC checks came through. 

Taking a closer look at the VCC and GND on the ESP8266 board I finally noticed the extremely dry pins and a failed soldering job. 

<img src="/images/mr_robot_v1-03.png"  width="auto" width="48%" alt="Macro shot of MCU board very close where zero solder is seen connecting pins to PCB"/> 
<img src="/images/mr_robot_v1-03.5.png"  width="auto" width="48%" alt="Another macro shot of MCU board very close where zero solder is seen connecting pins to PCB"/> 
Dry as a bone! 💀

It finally made sense.  In the rush to have the boards done in time for defcon, the pogo pin FTDI was used to hold the programmer in place and attempt a write. If the badge booted and showed "Mr Robot Badge", it got a serial number written on it and went into the "good" box. If it failed to write a couple times.... there wasn't any time to troubleshoot the badge so it went in the "bad" box for.... I dunno.... disposal?  It got an "X" serial number. Maybe they gave them away for free. Update: [Some of them took a trip](https://newscrewdriver.com/2022/04/07/surface-mount-repair-practice-with-mr-robot-badge/#:~:text=at%20a%20past%20Hackaday%20Superconference%2c%20I%20had%20the%20chance%20to%20play%20with%20a%20small%20batch%20of%20%22Mr.%20Robot%20Badge%22%20that%20were%20deemed%20defective%20for%20one%20reason%20or%20another%2e) to [Supercon!](https://hackaday.com/tag/supercon/)

Here's a valid badge rear... unlike mine:
<img src="/images/mr_robot_v1-04.jpeg" width="auto" width="100%" alt="Clean badge back showing serial number 103"/>

Time to bust out my new microscope and fix the pins. Almost done!

<img src="/images/mr_robot_v1-05.png" width="auto" width="100%" alt="Extreme macro shot showing fixed solder pins."/>

This is sooooooo much better than doing it by eye and occasionally bridging a pin, then wasting hours fixing a rough solder job.  Buy a scope. It's worth it. 

With the pins all cleaned up, the esptool was working correctly and was easily able to write the v1 firmware to the board.


<img src="/images/mr_robot_v1-06.png" width="auto" width="100%" alt="Terminal screenshot showing successful esptool run and output."/>

And we are..... GOOD!


<img src="/images/mr_robot_v1_med_1.png" width="auto" width="13%" alt="Scrolling 'Mr Robot Badge' text"/>
<img src="/images/mr_robot_v1_med_2.png" width="auto" width="13%" alt="Scrolling 'Mr Robot Badge' text"/>
<img src="/images/mr_robot_v1_med_3.png" width="auto" width="13%" alt="Scrolling 'Mr Robot Badge' text"/>
<img src="/images/mr_robot_v1_med_4.png" width="auto" width="13%" alt="Scrolling 'Mr Robot Badge' text"/>
<img src="/images/mr_robot_v1_med_5.png" width="auto" width="13%" alt="Scrolling 'Mr Robot Badge' text"/>
<img src="/images/mr_robot_v1_med_6.png" width="auto" width="13%" alt="Scrolling 'Mr Robot Badge' text"/>
<img src="/images/mr_robot_v1_med_7.png" width="auto" width="13%" alt="Scrolling 'Mr Robot Badge' text"/>

Notice the unique serial "X" on this resurrected badge. 😎  

Really glad I took a chance on this one. More board repair in the future.....

Update: Ok so [not _totally_ unique](https://www.worthpoint.com/worthopedia/official-defcon-25-mr-robot-badge-1881890346).  ([Archive](/images/mr_robot_v1_worthpoint.jpg)) 
