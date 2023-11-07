set term eps
set output "5G-nr.plt.eps"
set grid
set xrange [27949:28251]
set yrange [1:100]
set xlabel "f [MHz]"
set object 1 rect from 27950,70 to 28050,90 front fs empty 
LABEL1 = "n0"
set label 1 at 27950,80 LABEL1
set object 2 rect from 27950,40 to 28050,60 front fs empty 
LABEL2 = "CC0"
set label 2 at 27950,50 LABEL2
set object 3 rect from 27950,10 to 28050,30 front fs empty 
LABEL3 = "BWP0"
set label 3 at 27950,20 LABEL3
set object 4 rect from 28150,70 to 28250,90 front fs empty 
LABEL4 = "n1"
set label 4 at 28150,80 LABEL4
set object 5 rect from 28150,40 to 28250,60 front fs empty 
LABEL5 = "CC1"
set label 5 at 28150,50 LABEL5
set object 6 rect from 28150,10 to 28250,30 front fs empty 
LABEL6 = "BWP1"
set label 6 at 28150,20 LABEL6
unset key
plot -x
