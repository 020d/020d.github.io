---
layout: post
title:  "Defcon 20 Badge Revisited - Part 1"
date: 2024-08-12
---

Recently I managed to get ahold of a Defcon 20 badge. My addiction continues to worsen as my badge collection grows in size. 

Having missed out on the original conference I'm taking a retroactive look at the tech. 

Using the serial connection I'm able to see the badge status and how far along the challenge the previous owner got. 

This badge is using IR codes to identify each badge role. The idea is to encourage moving around the conference and meeting different people, vendors, staff, goons, artists, etc... 

Once all the different badge types are seen by the badge it's challenge is complete and it's unlocked showing the celebration sequence. 

Using a modern tool I thought I'd give it a shot with a flipper zero and see how easy it is to bypass this check and unlock my badge here at home.

Plugging into the USB connection and starting a serial console session I receive the following:

<pre><code>
=========================
  D E F C O N   2 0 1 2
=========================

Human............ No
 Artist.......... No
  Press.......... No
   Speaker....... No
    Vendor....... No
     Contest..... No
      Goon....... No
       Uber...... No

Status........... Sleeping it off

Happy 20th Defcon

1o57
</code></pre>

This signifies a lonely badge owner who hasn't met anyone. All badge types have a No. 

FYI the Badge uses the slightly unique Parallax Serial Terminal protocol so you'll get cleaner output using "Spin Tools IDE" or several other supported Parallax console tools. [Parallax dev tools list](https://www.parallax.com/propeller-2/programming-tools/)

The badge's normal loop is to check for other badge codes, and periodically transmit it's own. Let's catch this IR code with a Flipper Zero.

![Flipper Zero screen, IR files](/images/flipper_ir_files.jpg)

Quickly caught, and saved as a remote, we can pull the data off and edit.
<pre><code>
Filetype: IR signals file
Version: 1
# 
name: SIRC_00
type: parsed
protocol: SIRC
address: 08 00 00 00
command: 00 00 00 00
# 
</code></pre>

Copying the saved remote file off the Flipper, the .IR files are stored in a very simple text format. The protocol was reconized as [SIRC](https://www.sbprojects.net/knowledge/ir/sirc.php) using an address (or device) of 08 and a command of 0. 

Per the page referenced it looks like it's basically something like a CD player remote code for the 0 digit button. 

Not bad.  Let's play it back to the badge and see what happens.

<pre><code>
=========================
  D E F C O N   2 0 1 2
=========================

Human............ Yes
 Artist.......... No
  Press.......... No
   Speaker....... No
    Vendor....... No
     Contest..... No
      Goon....... No
       Uber...... No

Status........... Wallflower

Happy 20th Defcon

1o57
</code></pre>

There we have some progress. I've now "seen" another Human badge and one step of the badge is unlocked. 

Fooling around with the IR codes file it's apparent that a sequential code is used with remote button 1 being an Artist, 2 being Press, etc.... we can copy and paste the initial code and make a new button for each role, easily sending them one at a time.

<details>
<summary>Full .ir file for Flipper Zero</summary>
<pre><code>
Filetype: IR signals file
Version: 1
#
name: Human
type: parsed
protocol: SIRC
address: 08 00 00 00
command: 00 00 00 00
#
name: Artist
type: parsed
protocol: SIRC
address: 08 00 00 00
command: 01 00 00 00
#
name: Press
type: parsed
protocol: SIRC
address: 08 00 00 00
command: 02 00 00 00
#
name: Speaker
type: parsed
protocol: SIRC
address: 08 00 00 00
command: 03 00 00 00
#
name: Vendor
type: parsed
protocol: SIRC
address: 08 00 00 00
command: 04 00 00 00
#
name: Contest
type: parsed
protocol: SIRC
address: 08 00 00 00
command: 05 00 00 00
#
name: Goon
type: parsed
protocol: SIRC
address: 08 00 00 00
command: 06 00 00 00
#
name: Uber
type: parsed
protocol: SIRC
address: 08 00 00 00
command: 07 00 00 00
</code></pre>
</details>


Cool!  Now sending these I can watch the Serial response and see them unlocked one at a time, gradually increasing my badge's status as I'm no longer a wallflower.



### Full sequence of screens til Defcon 20 badge unlocked

``` 

=========================
  D E F C O N   2 0 1 2
=========================

Human............ Yes
 Artist.......... Yes
  Press.......... No
   Speaker....... No
    Vendor....... No
     Contest..... No
      Goon....... No
       Uber...... No

Status........... Slacker

Happy 20th Defcon

1o57

=========================
  D E F C O N   2 0 1 2
=========================

Human............ Yes
 Artist.......... Yes
  Press.......... Yes
   Speaker....... No
    Vendor....... No
     Contest..... No
      Goon....... No
       Uber...... No

Status........... Getting Around

Happy 20th Defcon

1o57

=========================
  D E F C O N   2 0 1 2
=========================

Human............ Yes
 Artist.......... Yes
  Press.......... Yes
   Speaker....... Yes
    Vendor....... No
     Contest..... No
      Goon....... No
       Uber...... No

Status........... Making Friends

Happy 20th Defcon

1o57

=========================
  D E F C O N   2 0 1 2
=========================

Human............ Yes
 Artist.......... Yes
  Press.......... Yes
   Speaker....... Yes
    Vendor....... Yes
     Contest..... No
      Goon....... No
       Uber...... No

Status........... Workin' the Con

Happy 20th Defcon

1o57

=========================
  D E F C O N   2 0 1 2
=========================

Human............ Yes
 Artist.......... Yes
  Press.......... Yes
   Speaker....... Yes
    Vendor....... Yes
     Contest..... Yes
      Goon....... No
       Uber...... No

Status........... Hustler

Happy 20th Defcon

1o57

=========================
  D E F C O N   2 0 1 2
=========================

Human............ Yes
 Artist.......... Yes
  Press.......... Yes
   Speaker....... Yes
    Vendor....... Yes
     Contest..... Yes
      Goon....... Yes
       Uber...... No

Status........... Big Schmooze

Happy 20th Defcon

1o57

=========================
  D E F C O N   2 0 1 2
=========================

Human............ Yes
 Artist.......... Yes
  Press.......... Yes
   Speaker....... Yes
    Vendor....... Yes
     Contest..... Yes
      Goon....... Yes
       Uber...... Yes

Status........... You are now 31337, well at least you've seen all badge types.  CODE: 10571089
Or fher gb qevax lbhe Binygvar.
Happy 20th Defcon

1o57
``` 


From Sleeping it off, to Wallflower, Slacker, Getting Around, Making Friends, Workin' the Con, Hustler, Big Schmooze to the final complete status.
All done!  Victory sequence now flashes. 

![Victory animation](https://020d.github.io/images/final_animation.mov)

Of course during the con 12 years ago nobody had a FZ, in part 2 I'll look at the Spin language and how to unlock a solo badge in 2024 without a Flipper Zero.
