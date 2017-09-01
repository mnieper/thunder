(closure
 (code
  `((entry)
    (movi %r0 1)
    (movi %r1 3)
    loop
    (beqi done %r1 0)
    (mulr %r0 %r0 %r1)
    (subi %r1 %r1 1)
    (jmpi loop)
    done
    (prepare)
    (pushargi "%d
")
    (ellipsis)
    (pushargr %r0)
    (finishi &printf)
    (movi %r0 0)
    (ret))))
