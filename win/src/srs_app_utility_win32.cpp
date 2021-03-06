#include <srs_app_utility.hpp>

#include <sys/types.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

using namespace std;

#include <srs_kernel_log.hpp>
#include <srs_app_config.hpp>
#include <srs_kernel_utility.hpp>
#include <srs_kernel_error.hpp>

#define SRS_LOCAL_LOOP_IP "127.0.0.1"

int srs_get_log_level(std::string level)
{
    if ("verbose" == _srs_config->get_log_level()) {
        return SrsLogLevel::Verbose;
    } else if ("info" == _srs_config->get_log_level()) {
        return SrsLogLevel::Info;
    } else if ("trace" == _srs_config->get_log_level()) {
        return SrsLogLevel::Trace;
    } else if ("warn" == _srs_config->get_log_level()) {
        return SrsLogLevel::Warn;
    } else if ("error" == _srs_config->get_log_level()) {
        return SrsLogLevel::Error;
    } else {
        return SrsLogLevel::Trace;
    }
}

static SrsRusage _srs_system_rusage;

SrsRusage::SrsRusage()
{
    ok = false;
    sample_time = 0;
    memset(&r, 0, sizeof(rusage));
}

SrsRusage* srs_get_system_rusage()
{
    return &_srs_system_rusage;
}

void srs_update_system_rusage()
{
    if (getrusage(RUSAGE_SELF, &_srs_system_rusage.r) < 0) {
        srs_warn("getrusage failed, ignore");
        return;
    }
        
    srs_update_system_time_ms();
    _srs_system_rusage.sample_time = srs_get_system_time_ms();
    
    _srs_system_rusage.ok = true;
}

static SrsProcSelfStat _srs_system_cpu_self_stat;
static SrsProcSystemStat _srs_system_cpu_system_stat;

SrsProcSelfStat::SrsProcSelfStat()
{
    ok = false;
    sample_time = 0;
    percent = 0;
    
    pid = 0;
    memset(comm, 0, sizeof(comm));
    state = 0;
    ppid = 0;
    pgrp = 0;
    session = 0;
    tty_nr = 0;
    tpgid = 0;
    flags = 0;
    minflt = 0;
    cminflt = 0;
    majflt = 0;
    cmajflt = 0;
    utime = 0;
    stime = 0;
    cutime = 0;
    cstime = 0;
    priority = 0;
    nice = 0;
    num_threads = 0;
    itrealvalue = 0;
    starttime = 0;
    vsize = 0;
    rss = 0;
    rsslim = 0;
    startcode = 0;
    endcode = 0;
    startstack = 0;
    kstkesp = 0;
    kstkeip = 0;
    signal = 0;
    blocked = 0;
    sigignore = 0;
    sigcatch = 0;
    wchan = 0;
    nswap = 0;
    cnswap = 0;
    exit_signal = 0;
    processor = 0;
    rt_priority = 0;
    policy = 0;
    delayacct_blkio_ticks = 0;
    guest_time = 0;
    cguest_time = 0;
}

SrsProcSystemStat::SrsProcSystemStat()
{
    ok = false;
    sample_time = 0;
    percent = 0;
    memset(label, 0, sizeof(label));
    user = 0;
    nice = 0;
    sys = 0;
    idle = 0;
    iowait = 0;
    irq = 0;
    softirq = 0;
    steal = 0;
    guest = 0;
}

SrsProcSelfStat* srs_get_self_proc_stat()
{
    return &_srs_system_cpu_self_stat;
}

SrsProcSystemStat* srs_get_system_proc_stat()
{
    return &_srs_system_cpu_system_stat;
}

#include <CpuInfo.h>
CpuInfo cpu = {0};

bool get_proc_system_stat(SrsProcSystemStat& r)
{
	if (NO_ERROR != CpuInfo_Read(&cpu))
	{
		r.ok = false;
	}
	else{
		strncpy(r.label, "cpu", sizeof("cpu"));
		r.user = -1;
		r.nice = cpu.cpu_nice;
		r.sys = cpu.cpu_system;
		r.idle = cpu.cpu_idle;
		r.iowait = cpu.cpu_wio;
		r.irq = cpu.cpu_intr;
		r.softirq = cpu.cpu_sintr;
		r.steal = -1;
		r.guest = -1;
		r.ok = true;
	}
	return r.ok;
}

bool get_proc_self_stat(SrsProcSelfStat& r)
{
#ifndef WIN32
	FILE* f = fopen("/proc/self/stat", "r");
	if (f == NULL) {
		srs_warn("open self cpu stat failed, ignore");
		return false;
	}

	int ret = fscanf(f, "%d %32s %c %d %d %d %d "
		"%d %u %lu %lu %lu %lu "
		"%lu %lu %ld %ld %ld %ld "
		"%ld %ld %llu %lu %ld "
		"%lu %lu %lu %lu %lu "
		"%lu %lu %lu %lu %lu "
		"%lu %lu %lu %d %d "
		"%u %u %llu "
		"%lu %ld", 
		&r.pid, r.comm, &r.state, &r.ppid, &r.pgrp, &r.session, &r.tty_nr,
		&r.tpgid, &r.flags, &r.minflt, &r.cminflt, &r.majflt, &r.cmajflt,
		&r.utime, &r.stime, &r.cutime, &r.cstime, &r.priority, &r.nice,
		&r.num_threads, &r.itrealvalue, &r.starttime, &r.vsize, &r.rss,
		&r.rsslim, &r.startcode, &r.endcode, &r.startstack, &r.kstkesp,
		&r.kstkeip, &r.signal, &r.blocked, &r.sigignore, &r.sigcatch,
		&r.wchan, &r.nswap, &r.cnswap, &r.exit_signal, &r.processor,
		&r.rt_priority, &r.policy, &r.delayacct_blkio_ticks, 
		&r.guest_time, &r.cguest_time);

	fclose(f);

	if (ret >= 0) {
		r.ok = true;
	}
#endif
	return r.ok;
}

void srs_update_proc_stat()
{
    srs_update_system_time_ms();
    
    // system cpu stat
    if (true) {
        SrsProcSystemStat r;
        if (!get_proc_system_stat(r)) {
            return;
        }
        
        r.sample_time = srs_get_system_time_ms();
        
        // calc usage in percent
        SrsProcSystemStat& o = _srs_system_cpu_system_stat;
        
        // @see: http://blog.csdn.net/nineday/article/details/1928847
        int64_t total = (r.user + r.nice + r.sys + r.idle + r.iowait + r.irq + r.softirq + r.steal + r.guest) 
            - (o.user + o.nice + o.sys + o.idle + o.iowait + o.irq + o.softirq + o.steal + o.guest);
        int64_t idle = r.idle - o.idle;
        if (total > 0) {
            r.percent = (float)(1 - idle / (double)total);
        }
        
        // upate cache.
        _srs_system_cpu_system_stat = r;
    }
    
    // self cpu stat
    if (true) {
        SrsProcSelfStat r;
        if (!get_proc_self_stat(r)) {
            return;
        }
        
        srs_update_system_time_ms();
        r.sample_time = srs_get_system_time_ms();
        
        // calc usage in percent
        SrsProcSelfStat& o = _srs_system_cpu_self_stat;
        
        // @see: http://stackoverflow.com/questions/16011677/calculating-cpu-usage-using-proc-files
        int64_t total = r.sample_time - o.sample_time;
        int64_t usage = (r.utime + r.stime) - (o.utime + o.stime);
        if (total > 0) {
            r.percent = (float)(usage * 1000 / (double)total / 100);
        }
        
        // upate cache.
        _srs_system_cpu_self_stat = r;
    }
}

SrsMemInfo::SrsMemInfo()
{
    ok = false;
    sample_time = 0;
    
    percent_ram = 0;
    percent_swap = 0;
    
    MemActive = 0;
    RealInUse = 0;
    NotInUse = 0;
    MemTotal = 0;
    MemFree = 0;
    Buffers = 0;
    Cached = 0;
    SwapTotal = 0;
    SwapFree = 0;
}

static SrsMemInfo _srs_system_meminfo;

SrsMemInfo* srs_get_meminfo()
{
    return &_srs_system_meminfo;
}

#include <MemInfo.h>

static MemInfo mem = {0,0,0,0,0,0,0,0,0,0,0};

void srs_update_meminfo()
{
	SrsMemInfo& r = _srs_system_meminfo;
	r.ok = false;

	if(MemInfo_Read(&mem) < 0){
		return;
	}
	
	r.MemTotal = mem.mem_total;
	r.MemFree = mem.mem_free;
	r.Buffers = mem.mem_buffers;
	r.Cached = mem.mem_cached;
	r.SwapTotal = mem.mem_cached;
	r.SwapFree = mem.swap_free;

	r.sample_time = srs_get_system_time_ms();
	r.MemActive = r.MemTotal - r.MemFree;
	r.RealInUse = r.MemActive - r.Buffers - r.Cached;
	r.NotInUse = r.MemTotal - r.RealInUse;

	r.ok = true;
	if (r.MemTotal > 0) {
		r.percent_ram = (float)(r.RealInUse / (double)r.MemTotal);
	}
	if (r.SwapTotal > 0) {
		r.percent_swap = (float)((r.SwapTotal - r.SwapFree) / (double)r.SwapTotal);
	}
}

SrsCpuInfo::SrsCpuInfo()
{
    ok = false;
    
    nb_processors = 0;
    nb_processors_online = 0;
}

SrsCpuInfo* srs_get_cpuinfo()
{
    static SrsCpuInfo* cpu = NULL;
    if (cpu != NULL) {
        return cpu;
    }
    
    // initialize cpu info.
    cpu = new SrsCpuInfo();
    cpu->ok = true;
    cpu->nb_processors = sysconf(_SC_NPROCESSORS_CONF);
    cpu->nb_processors_online = sysconf(_SC_NPROCESSORS_ONLN);
    
    return cpu;
}

SrsPlatformInfo::SrsPlatformInfo()
{
    ok = false;
    
    srs_startup_time = srs_get_system_time_ms();
    
    os_uptime = 0;
    os_ilde_time = 0;
    
    load_one_minutes = 0;
    load_five_minutes = 0;
    load_fifteen_minutes = 0;
}

static SrsPlatformInfo _srs_system_platform_info;

SrsPlatformInfo* srs_get_platform_info()
{
    return &_srs_system_platform_info;
}

#include <LoadAvg.h>
#include <UpTime.h>

static LoadAvg la = {0.0f, 0.0f, 0.0f, NULL, NULL};
static UpTime ut = {0};

void srs_update_platform_info()
{
	SrsPlatformInfo& r = _srs_system_platform_info;
	r.ok = true;

	if (true) {
		if (NO_ERROR != UpTime_Init(&ut)){
			r.ok = false;
		}
		if(NO_ERROR != UpTime_Read(&ut)){
			r.ok = false;
		}
		else{
			r.os_uptime = ut.SystemTime;
			r.os_ilde_time = ut.IdleTime;
		}
	}

	if (true) {
		if(LoadAvg_CalcLoad(&la) < 0){
			r.ok = false;
			return;
		}
		r.load_one_minutes = la.load_1;
		r.load_five_minutes = la.load_5;
		r.load_fifteen_minutes = la.load_15;
	}
}
vector<string> _srs_system_ipv4_ips;

void retrieve_local_ipv4_ips()
{
	vector<string>& ips = _srs_system_ipv4_ips;

	ips.clear();

	ifaddrs* ifap;
	if (getifaddrs(&ifap) == -1) {
		srs_warn("retrieve local ips, ini ifaddrs failed.");
		return;
	}

	ifaddrs* p = ifap;
	while (p != NULL) {
		sockaddr* addr = p->ifa_addr;

		// retrieve ipv4 addr
		if (addr->sa_family == AF_INET) {
			in_addr* inaddr = &((sockaddr_in*)addr)->sin_addr;

			char buf[16];
			memset(buf, 0, sizeof(buf));

			if ((inet_ntop(addr->sa_family, inaddr, buf, sizeof(buf))) == NULL) {
				srs_warn("convert local ip failed");
				break;
			}

			std::string ip = buf;
			if (ip != SRS_LOCAL_LOOP_IP) {
				srs_trace("retrieve local ipv4 addresses: %s", ip.c_str());
				ips.push_back(ip);
			}
		}

		p = p->ifa_next;
	}

	freeifaddrs(ifap);
}

vector<string>& srs_get_local_ipv4_ips()
{
	if (_srs_system_ipv4_ips.empty()) {
		retrieve_local_ipv4_ips();
	}

	return _srs_system_ipv4_ips;
}

string srs_get_local_ip(int fd)
{
	std::string ip;

	// discovery client information
	sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	if (getsockname(fd, (sockaddr*)&addr, &addrlen) == -1) {
		return ip;
	}
	srs_verbose("get local ip success.");

	// ip v4 or v6
	char buf[INET6_ADDRSTRLEN];
	memset(buf, 0, sizeof(buf));

	if ((inet_ntop(addr.sin_family, &addr.sin_addr, buf, sizeof(buf))) == NULL) {
		return ip;
	}

	ip = buf;

	srs_verbose("get local ip of client ip=%s, fd=%d", buf, fd);

	return ip;
}

string srs_get_peer_ip(int fd)
{
	std::string ip;

	// discovery client information
	sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	if (getpeername(fd, (sockaddr*)&addr, &addrlen) == -1) {
		return ip;
	}
	srs_verbose("get peer name success.");

	// ip v4 or v6
	char buf[INET6_ADDRSTRLEN];
	memset(buf, 0, sizeof(buf));

	if ((inet_ntop(addr.sin_family, &addr.sin_addr, buf, sizeof(buf))) == NULL) {
		return ip;
	}
	srs_verbose("get peer ip of client ip=%s, fd=%d", buf, fd);

	ip = buf;

	srs_verbose("get peer ip success. ip=%s, fd=%d", ip, fd);

	return ip;
}