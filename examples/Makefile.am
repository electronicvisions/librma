pc# (C) 2008, Mondrian Nuessle, Computer Architecture Group,             
## University of Heidelberg, Germany                                    
## (C) 2012, Tobias Groschup, University of Heidelberg, Germany
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
bin_PROGRAMS = send_recv_example

AM_CFLAGS = \
  -I$(top_srcdir)/include \
  @EXTOLL2DRV_INCLUDES@

send_recv_example_SOURCES = send_recv_example.c
send_recv_example_LDADD = $(top_builddir)/src/librma2.la
