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
bin_PROGRAMS = openclose connect rra_write rra_read pmem_write pmem_read pmem_read_with_noti mem_read_with_noti pmem_write_with_noti pmem_write_with_noti_irq mem_write_with_noti mem_write_with_noti_irq noti_put destination_process pmem_target testregister mem_read mem_write rma2test notiput_irq illegal_cmd illegal_cmd_with_noti illegal_mem_vpid_write illegal_mem_write illegal_vpid_write illegal_mem_write_requester illegal_mem_read illegal_excel_rra illegal_excel mem_read_ntr mem_write_ntr

AM_CFLAGS = \
  -I$(top_srcdir)/include \
  @EXTOLL2DRV_INCLUDES@

openclose_SOURCES = openclose.c
openclose_LDADD = $(top_builddir)/src/librma2.la

connect_SOURCES = connect.c
connect_LDADD = $(top_builddir)/src/librma2.la

rra_write_SOURCES = rra_write.c
rra_write_LDADD = $(top_builddir)/src/librma2.la

rra_read_SOURCES = rra_read.c
rra_read_LDADD = $(top_builddir)/src/librma2.la

pmem_write_SOURCES = pmem_write.c
pmem_write_LDADD = $(top_builddir)/src/librma2.la

pmem_read_SOURCES = pmem_read.c
pmem_read_LDADD = $(top_builddir)/src/librma2.la

mem_read_SOURCES = mem_read.c
mem_read_LDADD = $(top_builddir)/src/librma2.la

mem_write_SOURCES = mem_write.c
mem_write_LDADD = $(top_builddir)/src/librma2.la

mem_read_ntr_SOURCES = mem_read_ntr.c
mem_read_ntr_LDADD = $(top_builddir)/src/librma2.la

mem_write_ntr_SOURCES = mem_write_ntr.c
mem_write_ntr_LDADD = $(top_builddir)/src/librma2.la

pmem_read_with_noti_SOURCES = pmem_read_with_noti.c
pmem_read_with_noti_LDADD = $(top_builddir)/src/librma2.la

mem_read_with_noti_SOURCES = mem_read_with_noti.c
mem_read_with_noti_LDADD = $(top_builddir)/src/librma2.la

pmem_write_with_noti_SOURCES = pmem_write_with_noti.c
pmem_write_with_noti_LDADD = $(top_builddir)/src/librma2.la

pmem_write_with_noti_irq_SOURCES = pmem_write_with_noti_irq.c
pmem_write_with_noti_irq_LDADD = $(top_builddir)/src/librma2.la

mem_write_with_noti_SOURCES = mem_write_with_noti.c
mem_write_with_noti_LDADD = $(top_builddir)/src/librma2.la

mem_write_with_noti_irq_SOURCES = mem_write_with_noti_irq.c
mem_write_with_noti_irq_LDADD = $(top_builddir)/src/librma2.la

noti_put_SOURCES = noti_put.c
noti_put_LDADD = $(top_builddir)/src/librma2.la

destination_process_SOURCES = destination_process.c
destination_process_LDADD = $(top_builddir)/src/librma2.la

pmem_target_SOURCES = pmem_target.c
pmem_target_LDADD = $(top_builddir)/src/librma2.la

testregister_SOURCES = testregister.c
testregister_LDADD = $(top_builddir)/src/librma2.la

rma2test_SOURCES = rma2test.c rma2_ping.c rma2_pong.c rma2test.h
rma2test_LDADD = $(top_builddir)/src/librma2.la

notiput_irq_SOURCES = notiput_irq.c
notiput_irq_LDADD = $(top_builddir)/src/librma2.la

illegal_cmd_SOURCES = illegal_cmd.c
illegal_cmd_LDADD = $(top_builddir)/src/librma2.la

illegal_cmd_with_noti_SOURCES = illegal_cmd_with_noti.c
illegal_cmd_with_noti_LDADD = $(top_builddir)/src/librma2.la

illegal_mem_write_SOURCES = illegal_mem_write.c
illegal_mem_write_LDADD = $(top_builddir)/src/librma2.la

illegal_mem_write_requester_SOURCES = illegal_mem_write_requester.c
illegal_mem_write_requester_LDADD = $(top_builddir)/src/librma2.la

illegal_mem_vpid_write_SOURCES = illegal_mem_vpid_write.c
illegal_mem_vpid_write_LDADD = $(top_builddir)/src/librma2.la

illegal_vpid_write_SOURCES = illegal_vpid_write.c
illegal_vpid_write_LDADD = $(top_builddir)/src/librma2.la

illegal_mem_read_SOURCES = illegal_mem_read.c
illegal_mem_read_LDADD = $(top_builddir)/src/librma2.la

illegal_excel_rra_SOURCES = illegal_excel_rra.c
illegal_excel_rra_LDADD = $(top_builddir)/src/librma2.la

illegal_excel_SOURCES = illegal_excel.c
illegal_excel_LDADD = $(top_builddir)/src/librma2.la

