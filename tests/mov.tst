(closure
 (code
  '((entry)
    (prepare)
    (movi %r0 label)
    (jmpr %r0)
    label
    (pushargi "ok
")
    (ellipsis)
    (finishi &printf)
    (movi %r0 0)
    (ret))))
