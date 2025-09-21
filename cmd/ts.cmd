##turnstile demo test

turnstileInit();

turnstileShow();

#turnstileMsgSim(msgId, nInst)
turnstileMsgSim(1, 1);
turnstileMsgSim(2, 1);
turnstileMsgSim(3, 1);
turnstileMsgSim(4, 1);
turnstileMsgSim(1, 1);
turnstileMsgSim(2, 1);
turnstileMsgSim(2, 1);

turnstileInstanceShow(1);
turnstileFsmTraceShow(1);

turnstileMsgSim(1, 2);
turnstileMsgSim(2, 2);
turnstileMsgSim(2, 2);
turnstileMsgSim(1, 2);
turnstileMsgSim(1, 2);
turnstileMsgSim(2, 2);
turnstileMsgSim(1, 2);

turnstileInstanceShow(2);
turnstileFsmTraceShow(2);

