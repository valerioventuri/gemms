#!/bin/bash

echo MIGRAZIONE DEI FILE CREATI IN PRECEDENZA;
sleep 10

ssh tsm-hsm-13 "dsmmigrate  /gpfs/gemss_test/test/data1/greg*"

ssh tsm-hsm-12 "dsmmigrate  /gpfs/gemss_test/test/data2/greg*"

ssh tsm-hsm-13 "dsmmigrate  /gpfs/gemss_test2/test/data3/greg*"

ssh tsm-hsm-12 "dsmmigrate  /gpfs/gemss_test2/test/data4/greg*"
