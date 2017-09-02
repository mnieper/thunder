(closure
 (code
  `((entry)
    (movi %r0 "ok")
    (jmpi label)
    (movi %r0 "fail")
    label
    (prepare)
    (pushargr %r0)
    (finishi &puts)
    (ret))))
