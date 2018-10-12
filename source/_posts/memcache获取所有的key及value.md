---
title: 6. memcache获取所有的key及value
date: 2016-04-18 09:32:56
tags: [nosql,memcache]
---

## 0. 特别注意：
    通过cachedump命令只能获取1M的数据，无法获取更多的数据。


## 1. 命令行来查看memcache的key及取值

1. stats slabs  查看有哪些slab

    STAT 18:chunk_size 4544
    STAT 18:chunks_per_page 230
    STAT 18:total_pages 1
    STAT 18:total_chunks 230
    STAT 18:used_chunks 0
    STAT 18:free_chunks 7
    STAT 18:free_chunks_end 223
    STAT 18:mem_requested 18446744073709551411
    STAT 18:get_hits 777
    STAT 18:cmd_set 103
    STAT 18:delete_hits 0
    STAT 18:incr_hits 0
    STAT 18:decr_hits 0
    STAT 18:cas_hits 0
    STAT 18:cas_badval 0
 
2. stats cachedump slab_id limit
    
    将slab为slab_id的数据展示出来。 limit是展示的个数，如果取值为0， 则不作限制（其实是限制了1m）


http://blog.elijaa.org/2010/12/24/understanding-memcached-stats-cachedump-command/
http://blog.elijaa.org/2010/05/21/memcached-telnet-command-summary/#slabs_stats

## 2. php代码来获取

        <?php
        $mcobj = new Memcache();
        $mcobj->addServer('xxx',9150);

        $to_mcobj = new Memcache();
        $to_mcobj->addServer('xxx', 21212);

        $allSlabs = $mcobj->getExtendedStats('slabs');
        $items    = $mcobj->getExtendedStats('items');
        foreach ($allSlabs as $server => $slabs) {
            foreach ($slabs as $slabId => $slabMeta) {
                $cdump = $mcobj->getExtendedStats('cachedump', (int)$slabId, 0);
                foreach ($cdump as $keys => $vals) {
                    foreach ((array)$vals as $k => $v) {
                        $real_val = $mcobj->get($k);
                        $set_res = $to_mcobj->set($k, $real_val);
                        error_log('|slabId='.$slabId.'set--'.'|key='.$k.'|val='.json_encode($real_val).'|res='.$set_res."\n",3,'/tmp/yk.log');
                    }
                }
           }
                                                                                                    }
