(define s (string #\H #\e #\l #\l #\o #\, #\space #\W #\o #\r #\l #\d #\! #\newline))
(closure
 (code
  `((entry)
    (prepare)
    (pushargi ,s)
    (ellipsis)
    (finishi &printf)
    (movi %r0 0)
    (ret))))
