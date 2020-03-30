from graphTools import *
from expTools import *
import os
from matplotlib.backends.backend_pdf import PdfPages

#Lecture du fichier d'experiences: 
df = openfile(path = "./plots/data/fichier_perf.csv", sepa=";")
#print(df)
        
#Rajout de colonne supplémentaire : 
#Pour calculer le débit, décommenter la ligne du dessous
#df['debit'] = (df['dim'] ** 2) * df['iterations'] / df['time']

#Séléction des lignes :
# df = df[(-df.threads.isin([8])) & (df.kernel.isin(['mandel']))].reset_index(drop = True)

#Extraction des constantes :
constDico = {} #Permet de garder en mémoire les constantes, même après suppressions des colonnes
constStr = extractionConstante(constDico, df) #Utile pour afficher les constantes dans le titre

#print(constStr)
 

#Création du graphe : 
fig = creerGraphique(df = df,
	       constDico = constDico,
               constStr = constStr,
               x = 'threads',
               y = 'speedup',
               col ='dim',
               row = 'kernel',
               plottype = 'lineplot' #, yscale = 'log'
                  ) 
pp = PdfPages('speedUP.pdf')
plt.savefig(pp, format='pdf')
pp.close()
plt.close()
