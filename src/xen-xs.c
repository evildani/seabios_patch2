/*
 * xenbus.c: static, synchronous, read-only xenbus client for hvmloader.
 *
 * Copyright (c) 2009 Tim Deegan, Citrix Systems (R&D) Ltd.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "xen.h" // hypercalls
#include "config.h" // CONFIG_*
#include "util.h"
#include "bitops2.h"
#include "memmap.h"


static struct xenstore_domain_interface *rings; /* Shared ring with dom0 */
static evtchn_port_t event;                     /* Event-channel to dom0 */
static char payload[XENSTORE_PAYLOAD_MAX + 1];  /* Unmarshalling area */
void test_xenstore(void);


/*
 * a corresponds to path
 * b is value
 * returns an ill-formed string with no null at the end.
 */
char * build_write_query(char * a,char *b)
{
	int size = strlen(a)+strlen(b)+2;
	char *res = malloc_high(size);
	memset(res,0,size);
	//dprintf(1,"string path: %s\n",a);
	memcpy(res,a,strlen(a)+1); //include the null in the copy
	memcpy(res+strlen(a)+1,b,strlen(b)+1); //+1 to include the null
	return res;
}

char * strconcat(char *dest, const char *src)
{
	size_t dest_len = strlen(dest);
	size_t i;
	char *ret = malloc_high(strlen(dest)+strlen(src)+1);
	for(i = 0 ; dest[i] != '\0' ; i++)
	{
		ret[i] = dest[i];
	}
	for (i = 0 ; src[i] != '\0' ; i++)
	{
		ret[dest_len + i] = src[i];
	}
	ret[dest_len + i] = '\0'; //null terminate the string
	free(dest);
	dest = ret;
	return ret;
}
/*
 * Connect our xenbus client to the backend.
 * Call once, before any other xenbus actions.
 */
void xenbus_setup(void)
{
	struct xen_hvm_param param;
	if (!CONFIG_XEN)
		return;

	/* Ask Xen where the xenbus shared page is. */
	param.domid = DOMID_SELF;
	param.index = HVM_PARAM_STORE_PFN;
	if (hypercall_hvm_op(HVMOP_get_param, &param))
		panic("Error on setup");
	rings = (void *) (unsigned long) (param.value << PAGE_SHIFT);

	/* Ask Xen where the xenbus event channel is. */
	param.domid = DOMID_SELF;
	param.index = HVM_PARAM_STORE_EVTCHN;
	if (hypercall_hvm_op(HVMOP_get_param, &param))
		panic("error on hypercall to define rings and channel");
	event = param.value;
	dprintf(1,"Xenbus rings @0x%lx, event channel %lu\n",
			(unsigned long) rings, (unsigned long) event);
	test_xenstore();
}

/*
 * Reset the xenbus connection
 */
void xenbus_shutdown(void)
{
	if (rings == NULL)
		panic("rings not defined");
	memset(rings, 0, sizeof *rings);
	memset(get_shared_info(), 0, 1024);
	rings = NULL;
}

/*
 * 1. Get xen shared info
 * 2. get the guest event handle
 * 3. while no events pending
 * 4 .issue a yield to the CPU until event arrives
 */
static void ring_wait(void)
{
	struct shared_info *shinfo = get_shared_info();
	struct sched_poll poll;

	memset(&poll, 0, sizeof(poll));
	set_xen_guest_handle(poll.ports, &event);
	poll.nr_ports = 1;
	dprintf(1,"evtchn_pending 0x%p , 0x%lx at event %d \n",shinfo->evtchn_pending,shinfo->evtchn_pending[event],event);
	int wait = test_and_clear_bit(event, shinfo->evtchn_pending);
	int ret = 1;
	while (!wait || ret){
		ret = hypercall_sched_op(SCHEDOP_poll, &poll);
		wait = test_and_clear_bit(event, shinfo->evtchn_pending);
		dprintf(1,"DEBUG bit clear is %d and ret %d\n",wait,ret);
	}
}

static void ring_wait2(void)
{
	dprintf(1,"Start sleeping wait\n");
    struct shared_info *shinfo = get_shared_info();
    struct sched_poll poll;
    memset(&poll, 0, sizeof(poll));
    set_xen_guest_handle(poll.ports, &event);
    poll.nr_ports = 1;
    int i = 0;
    int j = 0;
    while ( !test_and_clear_bit(event, shinfo->evtchn_pending) ){
    	if(i<3){
    		i++;
    		dprintf(1,"wait loop started\n");
    	}
    	hypercall_sched_op(SCHEDOP_poll, &poll);
    	if(j<3){
    		j++;
    		dprintf(1,"wait loop after hypercall started\n");
    	}
    }
    dprintf(1,"Weak up from sleep\n");
}

static void print_event_status(void){
	struct evtchn_status status;
	struct shared_info *shinfo = get_shared_info();
	status.dom = DOMID_SELF;
	status.port = event;
	status.status = -1;
	status.vcpu = -1;
	u8 vpu_mask = shinfo->vcpu_info[0].evtchn_upcall_mask;
	u8 evnt_pending = shinfo->vcpu_info[0].evtchn_upcall_pending;
	dprintf(1,"VCPU.0 evtchn_upcall_mask %d AND evntchn_upcall_pending %d\n",vpu_mask,evnt_pending);
	hypercall_event_channel_op(EVTCHNOP_status,&status);
	dprintf(1,"Event Chn %d Status %d VCPU %d.\n",status.port,status.status, status.vcpu);
	if(status.status == EVTCHNSTAT_unbound){
		dprintf(1,"Event Channel is un bound, can be bound to %d\n",status.u.unbound.dom);
	}else{
		dprintf(1,"Event is interdomain to dom %d port %d\n",status.u.interdomain.dom,status.u.interdomain.port);
	}
	int bit = test_bit(event, shinfo->evtchn_pending);
	dprintf(1,"Event bit is %d for event %d.\n",bit,event);
}


/*
 * Writes data to xenstore ring
 */
static void ring_write(char *data, u32 len)
{
	u32 part;

	if (len >= XENSTORE_PAYLOAD_MAX)
		panic("Write Error on RINGS, more data than available buffer");

	while (len)
	{
		while ((part = (XENSTORE_RING_SIZE - 1) -
				MASK_XENSTORE_IDX(rings->req_prod - rings->req_cons)) == 0) {
			ring_wait();
			//The ring is not empty or not ready
		}
		if (part > (XENSTORE_RING_SIZE - MASK_XENSTORE_IDX(rings->req_prod)))
			part = XENSTORE_RING_SIZE - MASK_XENSTORE_IDX(rings->req_prod);

		if (part > len) /* Don't write more than we were asked for */
			part = len;
		memcpy(rings->req + MASK_XENSTORE_IDX(rings->req_prod), data, part);
		barrier();
		rings->req_prod += part;
		len -= part;
	}
}

/*
 * reads response from xenstore ring
 */
static void ring_read(char *data, u32 len)
{
	u32 part;

	if (len >= XENSTORE_PAYLOAD_MAX)
		panic("RING READ ERROR, more data that buffer space on rings");

	while (len) {
		while ((part = MASK_XENSTORE_IDX(rings->rsp_prod -rings->rsp_cons)) == 0) {
			dprintf(1,"Stuck on wait: PART %d ==0 rsp_prod %d rsp_cons %d\n",part,MASK_XENSTORE_IDX(rings->rsp_prod),MASK_XENSTORE_IDX(rings->rsp_cons));
			ring_wait(); //The ring is not ready or not empty
		}
		/* Don't overrun the end of the ring */
		if (part > (XENSTORE_RING_SIZE - MASK_XENSTORE_IDX(rings->rsp_cons)))
			part = XENSTORE_RING_SIZE - MASK_XENSTORE_IDX(rings->rsp_cons);

		if (part > len) /* Don't read more than we were asked for */
			part = len;
		memcpy(data, rings->rsp + MASK_XENSTORE_IDX(rings->rsp_cons), part);
		barrier();
		rings->rsp_cons += part;
		len -= part;
	}
}


/*
 * Send a request and wait for the answer.
 * Returns 0 for success, or an errno for error.
 */
static int xenbus_send(u32 type, u32 len, char *data,
		u32 *reply_len, char **reply_data)
{
	struct xsd_sockmsg hdr;
	evtchn_send_t send;
	int i,ret;

	/* Not acceptable to use xenbus before setting it up */
	if (rings == NULL)
		panic("XENBUS rings not defined\n");

	/* Put the request on the ring */
	hdr.type = type;
	/* We only ever issue one request at a time */
	hdr.req_id = 222;
	/* We never use transactions */
	hdr.tx_id = 0;
	hdr.len = len;
	ring_write((char *) &hdr, sizeof hdr);
	ring_write(data, len);
	/* Tell the other end about the request */
	send.port = event;
	//print_ring();
	//print_event_status();
	ret = hypercall_event_channel_op(EVTCHNOP_send, &send);
	//print_event_status();
	//dprintf(1,"Hypercall event %d channel notification %d\n",event,ret);
	/* Properly we should poll the event channel now but that involves
	 * mapping the shared-info page and handling the bitmaps. */
	/* Pull the reply off the ring */
	ring_read((char *) &hdr, sizeof(hdr));
	ring_read(payload, hdr.len);
	/* For sanity's sake, nul-terminate the answer */
	payload[hdr.len] = '\0';
	/* Handle errors */
	if ( hdr.type == XS_ERROR )
	{
		*reply_len = 0;
		for ( i = 0; i < ((sizeof xsd_errors) / (sizeof xsd_errors[0])); i++ ){
			if ( !strcmp(xsd_errors[i].errstring, payload) ){
				return xsd_errors[i].errnum;
			}
		}
		return EIO;
	}
	*reply_data = payload;
	*reply_len = hdr.len;
	return hdr.type;
}

char* xenstore_watch(char *path, char * key){
	if (rings == NULL)
		panic("rings not defined");
	char *answer = NULL;
	u32 ans_len=0;
	if ( xenbus_send(XS_WATCH, strlen(path)+1, path, &ans_len, &answer)== XS_ERROR ){
		return NULL;
	}
	dprintf(1,"XEN_WATCH response: %s.\n",answer);
	//ring_wait2();
	dprintf(1,"I was waked by Xenstore event: %s.\n",answer);

	dprintf(1,"path is %s\n",path);
	dprintf(1,"key is %s\n",key);
	char *query=build_write_query(path,key);
	int i=0;
	dprintf(1,"For Watch query: \n");
	for(i=0;i<=strlen(path)+strlen(key)+1;i++){
		if(query[i]=='\0'){
			dprintf(1,"NULL");
		}else{
			dprintf(1,"%c",query[i]);
		}
	}
	dprintf(1,".\n");
	/* Include the nul in the request */
	if ( xenbus_send(XS_WATCH, strlen(path)+strlen(key)+2, query, &ans_len, &answer)== XS_ERROR ){
		return NULL;
	}
	/* We know xenbus_send() nul-terminates its answer, so just pass it on. */
	dprintf(1,"XEN_WATCH response: %s.\n",answer);
	//ring_wait2();
	dprintf(1,"I was waked by Xenstore event: %s.\n",answer);

	return answer;
}

/*
 * Read a xenstore key.  Returns a nul-terminated string (even if the XS
 * data wasn't nul-terminated) or NULL.  The returned string is in a
 * static buffer, so only valid until the next xenstore/xenbus operation.
 */
char * xenstore_read(char *path)
{
	if (rings == NULL)
		panic("rings not defined");
	u32 len = 0;
	char *answer = NULL;

	/* Include the nul in the request */
	if ( xenbus_send(XS_READ, strlen(path)+1, path, &len, &answer)== XS_ERROR ){
		return NULL;
	}
	/* We know xenbus_send() nul-terminates its answer, so just pass it on. */
	return answer;
}

/*
 * Read a xenstore directory.  Returns a nul-separeted and nul-terminated string (even if the XS
 * data wasn't nul-terminated) or NULL.  The returned string is in a
 * static buffer, so only valid until the next xenstore/xenbus operation.
 * ans_len will tell the caller the length of the response
 */
char * xenstore_directory(char *path, u32 *ans_len)
{
	if (rings == NULL)
		panic("rings not defined");
	char *answer = NULL;

	/* Include the nul in the request */
	if ( xenbus_send(XS_DIRECTORY, strlen(path)+1, path, ans_len, &answer)== XS_ERROR ){
		return NULL;
	}
	/* We know xenbus_send() nul-terminates its answer, so just pass it on. */
	return answer;
}

char * xenstore_write(char *path, char *value)
{
	if (rings == NULL)
		panic("rings not defined");
	char *answer = NULL;
	u32 ans_len=0;
	char *query=build_write_query(path,value);
	/* Include the nul in the request */
	if ( xenbus_send(XS_WRITE, strlen(path)+strlen(value)+1, query, &ans_len, &answer)== XS_ERROR ){
		return NULL;
	}
	/* We know xenbus_send() nul-terminates its answer, so just pass it on. */
	return answer;
}

void test_xenstore(void){
	char  * path = "device/vbd";
	u32 ans_len;
	char * query;
	char * res = xenstore_directory(path,&ans_len);
	path = strconcat(path,"/");
	path = strconcat(path,res); //change path once to add vbd-id
	path = strconcat(path,"/test");
	dprintf(1,"Write Path is: %s.\n",path);
	char *res2 = xenstore_write(path,res);
	query = build_write_query(path,res);
	dprintf(1,"Xenstore Write %s length: %d strlen: %d res: %s.\n",res,ans_len,strlen(res2),res2);
	res2 = xenstore_watch(path,path);
	dprintf(1,"Xenstore Watch %s length: %d strlen: %d res: %s.\n",res,ans_len,strlen(res2),res2);
	ring_wait2(); //user must generate event to test /local/domain/XX/device/vbd/768/test
	ring_wait2(); //user must generate event to test
	print_event_status();
}

