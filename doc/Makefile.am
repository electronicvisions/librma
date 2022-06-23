# (C) 2008, Mondrian Nuessle, Computer Architecture Group,             
# University of Heidelberg, Germany                                    
# (C) 2011, Mondrian Nuessle, EXTOLL GmbH, Germany                     
#                                                                      
# This program is free software; you can redistribute it and/or modify 
# it under the terms of the GNU Lesser General Public License as       
# published by the Free Software Foundation; either version 3 of the   
# License, or (at your option) any later version.                      
#                                                                      
# This program is distributed in the hope that it will be useful,      
# but WITHOUT ANY WARRANTY; without even the implied warranty of       
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        
# GNU Lesser General Public License for more details.                  
#                                                                      
# You should have received a copy of the GNU Lesser General Public     
# License along with this program; if not, write to the Free Software  
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 
# USA                                                                  
#                                                                      
# For informations regarding this file contact nuessle@uni-mannheim.de 

docdir = /doc/librma

#fix for debian package building with autotools
destionation = $(DESTDIR)$(prefix)

EXTRA_DIST = \
        doxyfile

all: doc

doc: doxyfile ../include/rma2.h
	bash -c doxygen doxyfile
	cd latex && make

install-data-local:
	echo "destdir: $(DESTDIR)" 
	echo "destionation: $(destination)" 
	$(mkinstalldirs) $(destionation)$(docdir)
	$(mkinstalldirs) $(destionation)$(docdir)/html
	$(mkinstalldirs) $(destionation)$(docdir)/pdf
	$(INSTALL_DATA) ./html/*.html $(destionation)$(docdir)/html
	$(INSTALL_DATA) ./html/*.png $(destionation)$(docdir)/html
	$(INSTALL_DATA) ./html/*.css $(destionation)$(docdir)/html
#	$(INSTALL_DATA) ./html/*.js $(destionation)$(docdir)/html
	$(INSTALL_DATA) ./latex/refman.pdf $(destionation)$(docdir)/pdf/extoll2_rma_reference_manual.pdf

uninstall-local:
	rm -rf $(destionation)$(docdir)/html
	rm -rf $(destionation)$(docdir)/latex

.PHONY:doc
