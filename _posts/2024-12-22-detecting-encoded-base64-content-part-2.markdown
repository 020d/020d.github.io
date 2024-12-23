---
layout: post
title:  "Detecting encoded base64 content - Part 2"
date:   2024-12-22 
---

Alright so you've viewed Part 1... your base64 knowledge is getting dangerously useful, perhaps some rulesets have been reviewed and tweaked for better fidelity.  Cool!

One problem. Case variations. Part 2 of this series gets into how we can detect any permutation of upper/lower case strings WITHOUT decoding the base64 content. With a solid understanding of the encoding and it's interactions with the ASCII character set we can use an unintuitive strategy to detect what may at first seem impossible.

Let's go back to the example of "powershell" being base64 encoded. First off let's use a fun method to generate all the case permutations. I love the idea of using upper and lower cases as an analog for each bit and simply running a loop through the count to generate each variation. 

```
#!/usr/bin/env python3
import sys

def generate_case_permutations(input_string):
    # Calculate the total number of permutations (2^n)
    n = len(input_string)
    total_permutations = 2 ** n
#    print(f"Total permutations: {total_permutations}")

    # Loop through all integers from 0 to total_permutations - 1
    for i in range(total_permutations):
        permutation = []
        for j in range(n):
            # Check the bit at position j (from right to left)
            if (i >> j) & 1:
                permutation.append(input_string[j].upper())
            else:
                permutation.append(input_string[j].lower())
        # Join and output the current permutation
        print(''.join(permutation))


# Example usage
input_str = sys.argv[1]
generate_case_permutations(input_str)
```

We can quickly check the output is correct by counting the lines of output, and then ensuring that each line is unique and the count is the same. Remembering that input must be sorted for uniq to work correctly we get....

```
% ./binary_permutation.py powershell|wc -l
    1024
% ./binary_permutation.py powershell|sort -u|wc -l
    1024
```

Let's generate our encodings in bash because we love the CLI.
```
% ./binary_permutation.py powershell > psperms.txt
% for line in `cat psperms.txt`; do echo -n $line | base64 >> encodedps.txt ; done
```

Now let's check what the values are for the first character.
```
% cat encodedps.txt|cut -c 1|sort -u
U
c
```

Interesting huh? Out of 1024 combinations there's only 2 possible starting characters. Let's look at this for each position. 
```
% cat encodedps.txt|cut -c 1|sort -u
U
c
% cat encodedps.txt|cut -c 2|sort -u
E
G
% cat encodedps.txt|cut -c 3|sort -u
9
% cat encodedps.txt|cut -c 4|sort -u
3
X
% cat encodedps.txt|cut -c 5|sort -u
R
Z
% cat encodedps.txt|cut -c 6|sort -u
V
X
% cat encodedps.txt|cut -c 7|sort -u
J
% cat encodedps.txt|cut -c 8|sort -u
T
z
% cat encodedps.txt|cut -c 9|sort -u
S
a
% cat encodedps.txt|cut -c 10|sort -u
E
G
% cat encodedps.txt|cut -c 11|sort -u
V
% cat encodedps.txt|cut -c 12|sort -u
M
s
% cat encodedps.txt|cut -c 13|sort -u
T
b
% cat encodedps.txt|cut -c 14|sort -u
A
% cat encodedps.txt|cut -c 15|sort -u
=
% cat encodedps.txt|cut -c 16|sort -u
=
```

So there's actually only 2 possibilities for most of the character positions, and some only a single character of output. Why does this happen?

If you look at the hex values for the ASCII char "A" it's the famous 0x41, as often used in buffer overflow attacks. But the designers of the ASCII table had a logic to it, and the lower case characters are exactly 32 positions over starting at 0x61. Looking at those values in binary we see the all important "case bit" is at the third position.

```
0100 0001 0x41
0110 0001 0x61
```

To avoid doing multiple comparison operations to check if a character is within A-Z, and then between a-z, at times assembly language users will simply check if that bit is on or off. Since the base64 encoding turns 3 octets into 4 sextets, let's see what effect input case has on the output.

```
p         o         w
0x70      0x6F      0x77      (ASCII hex)
0111 0000|0110 1111|0111 0111 (octets as nibbles)
011100|000110|111101|110111   (base64 aligned sextets)
010100|000100|111101|010111   (case shifted)
0101 0000|0100 1111|0101 0111 (case shifted nibbles)
0x50      0x4F      0x57      (case shifted ASCII hex)
P         O         W
```

We can see by how the bits line up that the 3rd character in a base64 encoded 4 character chunk has the special property of always remaining unchanged. The alignment works out so that the unchanged parts of two characters are overlapping, and their case bits have ended up in the 2nd and 4th output position. As demonstrated:

```
% echo -n pow|base64
cG93
% echo -n POW|base64
UE9X
```

Using a regular expression syntax we can write a rule that works with every possible encoded permutation.

`(c|U)(G|E)9(3|X)(Z|R)(X|V)J(z|T)(a|S)(G|E)V(s|M)(T|b)`

Aligned for 4 bytes for easier viewing:

```
(c|U)(G|E)9(3|X)
(Z|R)(X|V)J(z|T)
(a|S)(G|E)V(s|M)
(T|b)
```

Testing this against every permutation we can verify it's hitting each line successfully:

```
% cat encodedps.txt|grep -E "(c|U)(G|E)9(3|X)(Z|R)(X|V)J(z|T)(a|S)(G|E)V(s|M)(T|b)"|wc -l 
    1024
```

Combining the concepts in part 1 and part 2, it becomes possible to write 3 regular expressions that together are able to catch every variation at any offset. With further effort this can be turned into a script that takes text input and outputs the 3 rule variations needed. Previously I've implemented this script so that it would output a Suricata/Snort compatible signature for easily targeting multiple suspicious and malicious script functions. It works great for low latency detections and where the base64 encoded data is unable to be decoded on the fly. In other cases the suspect string may be deeper in the encoded data than the decoding buffer allows. 

To avoid false positives on the more common encodings, consider if all lower, or camelcase are worth excluding from alerts so that the more suspect random cases are treated with greater severity.


