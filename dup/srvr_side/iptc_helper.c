#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <libiptc/libiptc.h>

struct xtc_handle *g_h;
static int insert_rule (const char *table,
		const char *chain, 
		unsigned int src,
		int inverted_src,
		unsigned int dest,
		int inverted_dst,
		const char *target)
{
	struct
	{
		struct ipt_entry entry;
		struct xt_standard_target target;
	} entry;
	int ret = 1;

	g_h = iptc_init (table);
	if (!g_h)
	{
		fprintf (stderr, "Could not init IPTC library: %s\n", iptc_strerror (errno));
		goto out;
	}

	memset (&entry, 0, sizeof (entry));

	/* target */
	entry.target.target.u.user.target_size = XT_ALIGN (sizeof (struct xt_standard_target));
	strncpy (entry.target.target.u.user.name, target, sizeof (entry.target.target.u.user.name));

	/* entry */
	entry.entry.target_offset = sizeof (struct ipt_entry);
	entry.entry.next_offset = entry.entry.target_offset + entry.target.target.u.user.target_size;

	if (src)
	{
		printf("%s: blocking ping requests\n", __func__);
		entry.entry.ip.src.s_addr  = src;
		entry.entry.ip.smsk.s_addr = 0xFFFFFFFF;
		if (inverted_src)
			entry.entry.ip.invflags |= IPT_INV_SRCIP;
		entry.entry.ip.proto = IPPROTO_ICMP;
	}

	if (dest)
	{
		entry.entry.ip.dst.s_addr  = dest;
		entry.entry.ip.dmsk.s_addr = 0xFFFFFFFF;
		if (inverted_dst)
			entry.entry.ip.invflags |= IPT_INV_DSTIP;
	}

	if (!iptc_append_entry (chain, (struct ipt_entry *) &entry, g_h))
	{
		fprintf (stderr, "Could not insert a rule in iptables (table %s): %s\n", table, iptc_strerror (errno));
		goto out;
	}

	if (!iptc_commit (g_h))
	{
		fprintf (stderr, "Could not commit changes in iptables (table %s): %s\n", table, iptc_strerror (errno));
		goto out;
	}

	ret = 0;
out:
	if (g_h)
		iptc_free (g_h);
	return ret;
}

void iptc_helper_add_rule(unsigned char *ipa, unsigned char *proto, unsigned char* action)
{
	unsigned int a, b;
	unsigned char ipaddr[16] = {0};
	sprintf(ipaddr, "%d.%d.%d.%d", ipa[0], ipa[1], ipa[2], ipa[3]);
	printf("source ipaddress is %s\n", ipaddr);
	b = inet_pton (PF_INET, ipaddr, &a);

	if (!b)
	{
		fprintf(stderr, "inet_pton failed because (%s)\n", strerror(errno));
	}
	if (!strncmp(action, "block", strlen("block")))
	{
		insert_rule ("filter", "INPUT", a, 0, 0, 0, "DROP");
	}
	else
	{
		if (g_h)
		{
			printf("%s: flushing all entries\n", __func__);
			g_h = iptc_init ("filter");
			if (!g_h)
			{
				fprintf (stderr, "Could not init IPTC library: %s\n", iptc_strerror (errno));
			}
			iptc_flush_entries("INPUT", g_h);
			if (!iptc_commit (g_h))
			{
				fprintf (stderr, "Could not commit changes in iptables: %s\n", iptc_strerror (errno));
			}
		}
	}
}
