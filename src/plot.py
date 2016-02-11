
import numpy as np
import matplotlib.pyplot as plt
import sys
#import csv
#import time

fig, ax = plt.subplots(1,1)
print ax
idx = 0
old = map(float, raw_input().split(','))
#old = 0
lines = [[None for col in range(3)] for row in range(2)]
#for i,axes in enumerate(ax):
#    colors = ['r','g','b']
#    for j,axis in enumerate(axes):
#        lines[i][j], = axis.plot([],[])
#        plt.setp(lines[i][j], linewidth=1, color=colors[j])

lines, = ax.plot([], [])
plt.setp(lines, linewidth=2, color='r')

t = np.arange(-30,0)
fx = np.zeros(30)
fy = np.zeros(30)
fz = np.zeros(30)

#for axes in ax:
#    for axis in axes:
#        axis.set_ylim(-20,20)
ax.set_ylim(-300,300)

## raw_input / csv.reader have not enough performance
while 1:
  s = sys.stdin.readline()
  l = map(float, s.split(','))
  #l = map(float, raw_input().split(','))
#reader = csv.reader(sys.stdin)
#for l in reader:
  #print "Read: (%s) %r" % (time.time(), l[1])

  t = np.roll(t,-1)
  t[29] = idx
  fx = np.roll(fx,-1)
  fx[29] = float(l[1])
  fy = np.roll(fy,-1)
  fy[29] = float(l[1])
  fz = np.roll(fz,-1)
  fz[29] = float(l[1])
  #print fx

  lines.set_data(t, fx * 1000 / 9.8) # N --> g
  #lines[0][0].set_data(t, fx)
  #lines[0][1].set_data(t, fy)
  #lines[0][2].set_data(t, fz)

  #for axes in ax:
  #    for axis in axes:
  #        axis.set_xlim(t.min(), t.max())
  #ax[0][0].set_xlim(t.min(), t.max())
  ax.set_xlim(t.min(), t.max())

  #ax.set_xlim(t.min(), t.max())
  #ax.set_ylim(min([-1.0, fx.min()]), max([1.0, fx.max()]))

  idx += 1
  #print l

  plt.pause(0.0001)


