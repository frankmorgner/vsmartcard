/* Taken from https://memset.wordpress.com/2010/10/09/inet_ntop-for-win32/ */

const char* inet_ntop(int af, const void* src, char* dst, int cnt){

    struct sockaddr_in srcaddr;

    memset(&srcaddr, 0, sizeof(struct sockaddr_in));
    memcpy(&(srcaddr.sin_addr), src, sizeof(srcaddr.sin_addr));

    srcaddr.sin_family = af;
    if (WSAAddressToString((struct sockaddr*) &srcaddr, sizeof(struct sockaddr_in), 0, dst, (LPDWORD) &cnt) != 0) {
        return NULL;
    }
    return dst;
}
