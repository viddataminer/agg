
int nslookup(char *in_nm_or_ip, char *out_nm_or_ip)
{
    FILE *pipe;
    long got_line=0;
    char *sptr, buf[1000], *p;
    int in_type;

    sprintf(buf,"nslookup %s", in_nm_or_ip);
    if(!(pipe = popen(buf,"r"))) {
        return 1;
    }

    // is it a name, or an ip address?
    for (p = in_nm_or_ip; *p; ++p) if(!isdigit(*p) && *p != '.') break;
    in_type = *p ? NAME : IP;

    while (fgets(buf, 4999, pipe)) {
        got_line++;
        if(in_type == IP) {
            if(sptr=strstr(buf,"name =")) {
                sptr+=7;
                strcpy(out_nm_or_ip,sptr);
                if(sptr=strchr(out_nm_or_ip, '\n'))
                     *(--sptr) = '\0'; // get rid of '.' and \n at end of name
                pclose(pipe);
                return 0;
            }
        } else if(in_type == NAME) {
            if(!strncmp(buf,"Name:", 5)) {
                fgets(buf, 4999, pipe);
                if(!strncmp(buf,"Address:", 8)) {
                    strcpy(out_nm_or_ip,&buf[9]);
                    REMOVE_NEWLINE(out_nm_or_ip);
                    pclose(pipe);
                    return 0;
                } else {
                    printf("ERROR.. Address: does not follow Name:\n");
                    return 4;
                }
            }
        } else printf("Unknown IN_TYPE = %d\n",in_type);
    }
    pclose(pipe);
    if(got_line) return 2;
    return 3;
}                                                             83,5          76%

