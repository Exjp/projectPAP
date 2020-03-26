#!/usr/bin/env python3

from graphTools import *
import sys

args = parserArguments(sys.argv)
df = lireDataFrame(args)


# Rajout de colonne supplémentaire pour calculer le débit :
if args.y == "throughput":
    args.y = 'throughput (MPixel / s)'
    df[args.y] = (df['dim'] ** 2) * df['iterations'] / df['time']

# Select some lines
# df = df[(-df.threads.isin([8])) & (df.kernel.isin(['mandel']))].reset_index(drop = True)

# delete some columns
# del df['machine']

# print the data
# print(df)


# Sélection des lignes :
# df = df[(-df.threads.isin([8])) & (df.kernel.isin(['mandel']))].reset_index(drop = True)

# Création du graphe :
fig = creerGraphique(df=df,
                     x=args.x,
                     y=args.y,
                     col=args.col,
                     row=args.row,
                     plottype=args.plottype,
                     yscale=args.yscale,
                     xscale=args.xscale,
                     height=args.height,
                     showParameters=args.showParameters)

engeristrerGraphique(fig)
