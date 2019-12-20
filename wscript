def options(opt):
    opt.load('compiler_c')
    opt.load('compiler_cxx')


def configure(cfg):
    cfg.load('compiler_c')
    cfg.load('compiler_cxx')

    cfg.check_cc(header_name='pmap.h', includes='/opt/extoll/extoll-driver', uselib_store='PMAP4RMA2')
    cfg.check_cc(lib='dl', uselib_store='DL4RMA2')

def build(bld):
    bld(target          = 'librma_inc',
        export_includes = 'include'
    )

    bld.shlib(
        target       = 'rma2',
        features     = 'cxx',
        source       = ['src/librma2.c', 'src/extoll2_list.c'],
        use          = ['librma_inc', 'PMAP4RMA2', 'DL4RMA2'],
        install_path = '${PREFIX}/lib',
    )

    bld.shlib(
        target       = 'rma2rc',
        features     = 'cxx',
        source       = ['src/librma2rc.c'],
        use          = ['librma_inc', 'PMAP4RMA2', 'DL4RMA2'],
        install_path = '${PREFIX}/lib',
    )
