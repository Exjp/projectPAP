#!/usr/bin/env python3

from graphTools import *
import sys


df = lireDataFrame(sys.argv)

# Rajout de colonne supplémentaire pour calculer le débit :
throughput = 'throughput (MPixel / s)'
df[throughput] = (df['dim'] ** 2) * df['iterations'] / df['time']

# Select lines
# df = df[(-df.threads.isin([8])) & (df.kernel.isin(['mandel']))].reset_index(drop = True)

# print the data
# print(df)

# Création du graphe :
fig = creerGraphique(df=df,
                     x='threads',
                     y=throughput,
                     row=None,
                     col='kernel',
                     height=8,
                     plottype='lineplot'  # , yscale = 'log'
                     )

engeristrerGraphique(fig)
