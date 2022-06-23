## (C) 2008, Mondrian Nuessle, Computer Architecture Group,             
## University of Heidelberg, Germany                                    
## (C) 2011, Mondrian Nuessle, EXTOLL GmbH, Germany                     
##                                                                      
## This program is free software; you can redistribute it and/or modify 
## it under the terms of the GNU Lesser General Public License as       
## published by the Free Software Foundation; either version 3 of the   
## License, or (at your option) any later version.                      
##                                                                      
## This program is distributed in the hope that it will be useful,      
## but WITHOUT ANY WARRANTY; without even the implied warranty of       
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        
## GNU Lesser General Public License for more details.                  
##                                                                      
## You should have received a copy of the GNU Lesser General Public     
## License along with this program; if not, write to the Free Software  
## Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 
## USA                                                                  
##                                                                      
## For informations regarding this file contact nuessle@uni-mannheim.de 

## Process this file with automake to produce Makefile.in
ACLOCAL_AMFLAGS="-I m4"
lib_LTLIBRARIES = librma2.la librma2rc.la

AM_CFLAGS = -Wall -O2 -fPIC -I$(top_srcdir)/include @EXTOLL2DRV_INCLUDES@


librma2_la_SOURCES = librma2.c extoll2_list.c ../include/rma2.h
librma2rc_la_SOURCES = librma2rc.c ../include/rma2.h

## Remember to bump library versions before a release.
## Suppose the old version was CURRENT:REVISION:AGE.
##
##	If you have                	|bump the version to
##-----------------------------------------------------------------
##	not changed the interface  	|CURRENT : REVISION+1 : AGE
##	(bug fixes)			|
##					|
##	augmented the interface    	|CURRENT+1 : 0 : AGE+1
##	(new functions)			|
##					|
##	broken old interface       	|CURRENT+1 : 0 : 0
##	(e.g. removed functions)
librma2_la_LDFLAGS = -version-info 2:0:0 -Wl,-Bsymbolic
librma2rc_la_LDFLAGS = -version-info 2:0:0 -Wl,-Bsymbolic

