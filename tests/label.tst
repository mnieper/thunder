(closure
 (code
  `((entry)
    (movi %r0 "ok")
    (mulr %v0 %v1 %v2)
    (jmpi label)
    (movi %r0 "fail")
    label
    (prepare)
    (pushargr %r0)
    (finishi &puts)
    (ret))))
