/** 
 * @file nf_nat64_static_routes.c 
 *
 * This module implements the feature mentioned in the RFC6146, 
 * about managing static routes. It allows to add a new entry in
 * the BIB and Session tables from Userspace.
 */

#include "nf_nat64_config.h"
#include "nf_nat64_static_routes.h"
#include "nf_nat64_bib.h"
#include "nf_nat64_session.h"

enum response_code nat64_add_static_route(struct request_session *req)
{
	struct bib_entry *bib = NULL;
	struct session_entry *session = NULL;

	bib = nat64_create_bib_entry(&req->add.pair4.local, &req->add.pair6.remote);
	if (!bib) {
		log_warning("Could NOT allocate a BIB entry.");
		goto failure;
	}

	if (!nat64_add_bib_entry(bib, req->l4_proto)) {
		log_warning("Could NOT add the BIB entry to the table.");
		goto failure;
	}

	session = nat64_create_static_session_entry(&req->add.pair4, &req->add.pair6, bib,
			req->l4_proto);
	if (!session) {
		log_warning("Could NOT allocate a session entry.");
		goto failure;
	}

	if (!nat64_add_session_entry(session)) {
		log_warning("Could NOT add the session entry to the table.");
		goto failure;
	}

	return RESPONSE_SUCCESS;

failure:
	if (session)
		kfree(session);
	if (bib) {
		nat64_remove_bib_entry(bib, req->l4_proto);
		kfree(bib);
	}
	return RESPONSE_ALLOC_FAILED;
}

enum response_code nat64_delete_static_route(struct request_session *req)
{
	struct session_entry *session = NULL;

	switch (req->remove.l3_proto) {
	case NFPROTO_IPV6:
		session = nat64_get_session_entry_by_ipv6(&req->remove.pair6, req->l4_proto);
		break;
	case NFPROTO_IPV4:
		session = nat64_get_session_entry_by_ipv4(&req->remove.pair4, req->l4_proto);
		break;
	default:
		return RESPONSE_UNKNOWN_L3PROTO;
	}

	if (!session)
		return RESPONSE_NOT_FOUND;

	if (!nat64_remove_session_entry(session)) {
		log_err("Bug: Remove session call ended in failure, despite previous validations.");
		return RESPONSE_NOT_FOUND;
	}

	kfree(session);
	return RESPONSE_SUCCESS;
}

enum response_code nat64_print_bib_table(union request_bib *request, __u16 *count_out,
		struct bib_entry_us **bibs_us_out)
{
	struct bib_entry **bibs_ks = NULL; // ks = kernelspace. Array of pointers to bib entries.
	struct bib_entry_us *bibs_us = NULL; // us = userspace. Array of bib entries.
	__s32 counter, count;

	count = nat64_bib_to_array(request->display.l4_proto, &bibs_ks);
	if (count == 0) {
		*count_out = 0;
		*bibs_us_out = NULL;
		return RESPONSE_SUCCESS;
	}
	if (count < 0)
		goto kmalloc_fail;

	bibs_us = kmalloc(count * sizeof(*bibs_us), GFP_ATOMIC);
	if (!bibs_us)
		goto kmalloc_fail;

	for (counter = 0; counter < count; counter++) {
		bibs_us[counter].ipv4 = bibs_ks[counter]->ipv4;
		bibs_us[counter].ipv6 = bibs_ks[counter]->ipv6;
	}

	kfree(bibs_ks);
	*count_out = count;
	*bibs_us_out = bibs_us;
	return RESPONSE_SUCCESS;

kmalloc_fail:
	kfree(bibs_ks);
	return RESPONSE_ALLOC_FAILED;
}

enum response_code nat64_print_session_table(struct request_session *request, __u16 *count_out,
		struct session_entry_us **sessions_us_out)
{
	struct session_entry **sessions_ks = NULL;
	struct session_entry_us *sessions_us = NULL;
	__s32 counter, count;

	count = nat64_session_table_to_array(request->l4_proto, &sessions_ks);
	if (count == 0) {
		*count_out = 0;
		*sessions_us_out = NULL;
		return RESPONSE_SUCCESS;
	}
	if (count < 0)
		goto kmalloc_fail;

	sessions_us = kmalloc(count * sizeof(*sessions_us), GFP_ATOMIC);
	if (!sessions_us)
		goto kmalloc_fail;

	for (counter = 0; counter < count; counter++) {
		sessions_us[counter].ipv6 = sessions_ks[counter]->ipv6;
		sessions_us[counter].ipv4 = sessions_ks[counter]->ipv4;
		sessions_us[counter].is_static = sessions_ks[counter]->is_static;
		sessions_us[counter].dying_time = sessions_ks[counter]->dying_time;
		sessions_us[counter].l4protocol = sessions_ks[counter]->l4protocol;
	}

	kfree(sessions_ks);
	*count_out = count;
	*sessions_us_out = sessions_us;
	return RESPONSE_SUCCESS;

kmalloc_fail:
	kfree(sessions_ks);
	return RESPONSE_ALLOC_FAILED;
}