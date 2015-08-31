BEGIN {
    isEnd=1;
    print "snapshot, time, mem_heap_B, mem_heap_extra_B, mem_stacks_B";           
}
{

    if($1=="desc:")
        desc = $0;
    if($1=="cmd:"){
        cmd = $0;
        title = $6;
        }
    
    split($0,val,"=");
    snapshot[val[1]] = val[2];
    if(val[1]=="heap_tree"){
        printf("%s, %s, %s, %s, %s\n",snapshot["snapshot"], snapshot["time"],  snapshot["mem_heap_B"], snapshot["mem_heap_extra_B"], snapshot["mem_stacks_B"]);           
    }
    
        
    

}
END {
    printf("title, %s\n",title);
    print desc;
    print cmd;
}