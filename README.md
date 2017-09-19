# YAMemcached

Yet another memcached clone. 

Started as an effort to validate some of the ideas i had on how to develop a high performance server. 
Currently supports a working subset of memcached text protocol, UDP protocol is in development (but should be testable)

Performance can be tested via https://github.com/leverich/mutilate or https://github.com/antirez/mc-benchmark

When running mutilate with update rate of 0.5, consumes 60% less userspace CPU. Performance is still capped by lack of test cluster.
