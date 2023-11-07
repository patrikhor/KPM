set term eps
set output "5G-nr-1.plt.eps"
set grid
set xrange [27899:28701]
set yrange [1:100]
set xlabel "f [MHz]"
set object 1 rect from 27900,70 to 28100,90 front fs empty 
LABEL1 = "n0"
set label 1 at 27900,80 LABEL1
set object 2 rect from 27900,40 to 28000,60 front fs empty 
LABEL2 = "CC0"
set label 2 at 27900,50 LABEL2
set object 3 rect from 27900,10 to 28000,30 front fs empty 
LABEL3 = "BWP0"
set label 3 at 27900,20 LABEL3
set object 4 rect from 28000,40 to 28100,60 front fs empty 
LABEL4 = "CC1"
set label 4 at 28000,50 LABEL4
set object 5 rect from 28000,10 to 28100,30 front fs empty 
LABEL5 = "BWP1"
set label 5 at 28000,20 LABEL5
set object 6 rect from 28500,70 to 28700,90 front fs empty 
LABEL6 = "n1"
set label 6 at 28500,80 LABEL6
set object 7 rect from 28500,40 to 28600,60 front fs empty 
LABEL7 = "CC2"
set label 7 at 28500,50 LABEL7
set object 8 rect from 28500,10 to 28600,30 front fs empty 
LABEL8 = "BWP2"
set label 8 at 28500,20 LABEL8
set object 9 rect from 28600,40 to 28700,60 front fs empty 
LABEL9 = "CC3"
set label 9 at 28600,50 LABEL9
set object 10 rect from 28600,10 to 28700,30 front fs empty 
LABEL10 = "BWP3"
set label 10 at 28600,20 LABEL10
unset key
plot -x
