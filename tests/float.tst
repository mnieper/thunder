(closure
 (code
  `((entry)
    (movi_f %f0 1.0)
    (addr_f %f0 %f0 %f0)
    (addi_f %f0 %f0 2.0)
    (extr_f_d %f0 %f0)
    (prepare)
    (pushargi "Result: %g
")
    (ellipsis)
    (pushargr_d %f0)
    (finishi &printf)
    (movi %r0 0)
    (ret))))
