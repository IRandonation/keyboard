# keyboard

try to design my personal keyboard


## ROCK NUMBER KWYBOARD

### PCB design


###### ---------------2023.6.10--------------
As mt first keyboard, i try to design a small keyborad for number zone.
Not as same as regular keyboard, i tend to use stm32 instead of atmage.

###### ---------------2024.9.9----------------

Finish my PCB and successfully weld that. But what is so stupid is that i welded my shaft seat upside down, which led to i cannot put my shaft to my PCB. So i have to correct this problem.

###### ---------------2024.9.10---------------

i found a promblem that when you try to connnet stm32f103 to your computer by type-c, you should use two 5.1kR resistances in CC pin, which help computer to recognize the chip. For this version i did not use these resistance. Besides, you can ignore those two 2R2 resistances this go with D pin on type-c.
![STM32-TYPEC](Image\STM32_TYPEC.png)
