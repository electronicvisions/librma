def depends(dep):
    dep('extoll-driver')


def options(opt):
    opt.load('compiler_c')
    opt.load('compiler_cxx')


def configure(cfg):
    cfg.load('compiler_c')
    cfg.load('compiler_cxx')

    cfg.check_cc(lib='dl', uselib_store='DL4RMA2')

def build(bld):
    bld(target          = 'librma_inc',
        export_includes = 'include'
    )

    bld.shlib(
        target       = 'rma2',
        features     = 'cxx',
        source       = ['src/librma2.c', 'src/extoll2_list.c'],
        use          = ['librma_inc', 'PMAP4RMA2', 'DL4RMA2', 'extoll-driver_inc'],
        install_path = '${PREFIX}/lib',
    )

    bld.shlib(
        target       = 'rma2rc',
        features     = 'cxx',
        source       = ['src/librma2rc.c'],
        use          = ['librma_inc', 'PMAP4RMA2', 'DL4RMA2', 'extoll-driver_inc'],
        install_path = '${PREFIX}/lib',
    )

    bld.program(
        target       = 'librma-send_recv_example',
        features     = 'c',
        source       = ['examples/send_recv_example.c'],
        use          = ['librma_inc', 'rma2', 'rma2rc'],
        install_path = '${PREFIX}/bin',
    )

    for ex in ['read', 'write']:
        bld.program(
            target = 'librma-rra_{}'.format(ex),
            features = 'c',
            source = ['tests/rra_{}.c'.format(ex)],
            use = ['rma2rc', 'rma2', 'librma_inc'],
            install_path = '${PREFIX}/bin',
        )
