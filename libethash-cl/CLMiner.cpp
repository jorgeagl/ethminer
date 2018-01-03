/// OpenCL miner implementation.
///
/// @file
/// @copyright GNU General Public License

#include "CLMiner.h"
#include <libethash/internal.h>
#include "CLMiner_kernel.h"
#include <json/json.h>

using namespace dev;
using namespace eth;

namespace dev
{
namespace eth
{

unsigned CLMiner::s_workgroupSize = CLMiner::c_defaultLocalWorkSize;
unsigned CLMiner::s_initialGlobalWorkSize = CLMiner::c_defaultGlobalWorkSizeMultiplier * CLMiner::c_defaultLocalWorkSize;
unsigned CLMiner::s_threadsPerHash = 8;

constexpr size_t c_maxSearchResults = 1;

struct CLChannel: public LogChannel
{
	static const char* name() { return EthOrange " cl"; }
	static const int verbosity = 2;
	static const bool debug = false;
};
#define cllog clog(CLChannel)
#define ETHCL_LOG(_contents) cllog << _contents

/**
 * Returns the name of a numerical cl_int error
 * Takes constants from CL/cl.h and returns them in a readable format
 */
static const char *strClError(cl_int err) {

	switch (err) {
	case CL_SUCCESS:
		return "CL_SUCCESS";
	case CL_DEVICE_NOT_FOUND:
		return "CL_DEVICE_NOT_FOUND";
	case CL_DEVICE_NOT_AVAILABLE:
		return "CL_DEVICE_NOT_AVAILABLE";
	case CL_COMPILER_NOT_AVAILABLE:
		return "CL_COMPILER_NOT_AVAILABLE";
	case CL_MEM_OBJECT_ALLOCATION_FAILURE:
		return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
	case CL_OUT_OF_RESOURCES:
		return "CL_OUT_OF_RESOURCES";
	case CL_OUT_OF_HOST_MEMORY:
		return "CL_OUT_OF_HOST_MEMORY";
	case CL_PROFILING_INFO_NOT_AVAILABLE:
		return "CL_PROFILING_INFO_NOT_AVAILABLE";
	case CL_MEM_COPY_OVERLAP:
		return "CL_MEM_COPY_OVERLAP";
	case CL_IMAGE_FORMAT_MISMATCH:
		return "CL_IMAGE_FORMAT_MISMATCH";
	case CL_IMAGE_FORMAT_NOT_SUPPORTED:
		return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
	case CL_BUILD_PROGRAM_FAILURE:
		return "CL_BUILD_PROGRAM_FAILURE";
	case CL_MAP_FAILURE:
		return "CL_MAP_FAILURE";
	case CL_MISALIGNED_SUB_BUFFER_OFFSET:
		return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
	case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
		return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";

#ifdef CL_VERSION_1_2
	case CL_COMPILE_PROGRAM_FAILURE:
		return "CL_COMPILE_PROGRAM_FAILURE";
	case CL_LINKER_NOT_AVAILABLE:
		return "CL_LINKER_NOT_AVAILABLE";
	case CL_LINK_PROGRAM_FAILURE:
		return "CL_LINK_PROGRAM_FAILURE";
	case CL_DEVICE_PARTITION_FAILED:
		return "CL_DEVICE_PARTITION_FAILED";
	case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:
		return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";
#endif // CL_VERSION_1_2

	case CL_INVALID_VALUE:
		return "CL_INVALID_VALUE";
	case CL_INVALID_DEVICE_TYPE:
		return "CL_INVALID_DEVICE_TYPE";
	case CL_INVALID_PLATFORM:
		return "CL_INVALID_PLATFORM";
	case CL_INVALID_DEVICE:
		return "CL_INVALID_DEVICE";
	case CL_INVALID_CONTEXT:
		return "CL_INVALID_CONTEXT";
	case CL_INVALID_QUEUE_PROPERTIES:
		return "CL_INVALID_QUEUE_PROPERTIES";
	case CL_INVALID_COMMAND_QUEUE:
		return "CL_INVALID_COMMAND_QUEUE";
	case CL_INVALID_HOST_PTR:
		return "CL_INVALID_HOST_PTR";
	case CL_INVALID_MEM_OBJECT:
		return "CL_INVALID_MEM_OBJECT";
	case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
		return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
	case CL_INVALID_IMAGE_SIZE:
		return "CL_INVALID_IMAGE_SIZE";
	case CL_INVALID_SAMPLER:
		return "CL_INVALID_SAMPLER";
	case CL_INVALID_BINARY:
		return "CL_INVALID_BINARY";
	case CL_INVALID_BUILD_OPTIONS:
		return "CL_INVALID_BUILD_OPTIONS";
	case CL_INVALID_PROGRAM:
		return "CL_INVALID_PROGRAM";
	case CL_INVALID_PROGRAM_EXECUTABLE:
		return "CL_INVALID_PROGRAM_EXECUTABLE";
	case CL_INVALID_KERNEL_NAME:
		return "CL_INVALID_KERNEL_NAME";
	case CL_INVALID_KERNEL_DEFINITION:
		return "CL_INVALID_KERNEL_DEFINITION";
	case CL_INVALID_KERNEL:
		return "CL_INVALID_KERNEL";
	case CL_INVALID_ARG_INDEX:
		return "CL_INVALID_ARG_INDEX";
	case CL_INVALID_ARG_VALUE:
		return "CL_INVALID_ARG_VALUE";
	case CL_INVALID_ARG_SIZE:
		return "CL_INVALID_ARG_SIZE";
	case CL_INVALID_KERNEL_ARGS:
		return "CL_INVALID_KERNEL_ARGS";
	case CL_INVALID_WORK_DIMENSION:
		return "CL_INVALID_WORK_DIMENSION";
	case CL_INVALID_WORK_GROUP_SIZE:
		return "CL_INVALID_WORK_GROUP_SIZE";
	case CL_INVALID_WORK_ITEM_SIZE:
		return "CL_INVALID_WORK_ITEM_SIZE";
	case CL_INVALID_GLOBAL_OFFSET:
		return "CL_INVALID_GLOBAL_OFFSET";
	case CL_INVALID_EVENT_WAIT_LIST:
		return "CL_INVALID_EVENT_WAIT_LIST";
	case CL_INVALID_EVENT:
		return "CL_INVALID_EVENT";
	case CL_INVALID_OPERATION:
		return "CL_INVALID_OPERATION";
	case CL_INVALID_GL_OBJECT:
		return "CL_INVALID_GL_OBJECT";
	case CL_INVALID_BUFFER_SIZE:
		return "CL_INVALID_BUFFER_SIZE";
	case CL_INVALID_MIP_LEVEL:
		return "CL_INVALID_MIP_LEVEL";
	case CL_INVALID_GLOBAL_WORK_SIZE:
		return "CL_INVALID_GLOBAL_WORK_SIZE";
	case CL_INVALID_PROPERTY:
		return "CL_INVALID_PROPERTY";

#ifdef CL_VERSION_1_2
	case CL_INVALID_IMAGE_DESCRIPTOR:
		return "CL_INVALID_IMAGE_DESCRIPTOR";
	case CL_INVALID_COMPILER_OPTIONS:
		return "CL_INVALID_COMPILER_OPTIONS";
	case CL_INVALID_LINKER_OPTIONS:
		return "CL_INVALID_LINKER_OPTIONS";
	case CL_INVALID_DEVICE_PARTITION_COUNT:
		return "CL_INVALID_DEVICE_PARTITION_COUNT";
#endif // CL_VERSION_1_2

#ifdef CL_VERSION_2_0
	case CL_INVALID_PIPE_SIZE:
		return "CL_INVALID_PIPE_SIZE";
	case CL_INVALID_DEVICE_QUEUE:
		return "CL_INVALID_DEVICE_QUEUE";
#endif // CL_VERSION_2_0

#ifdef CL_VERSION_2_2
	case CL_INVALID_SPEC_ID:
		return "CL_INVALID_SPEC_ID";
	case CL_MAX_SIZE_RESTRICTION_EXCEEDED:
		return "CL_MAX_SIZE_RESTRICTION_EXCEEDED";
#endif // CL_VERSION_2_2
	}

	return "Unknown CL error encountered";
}

/**
 * Prints cl::Errors in a uniform way
 * @param msg text prepending the error message
 * @param clerr cl:Error object
 *
 * Prints errors in the format:
 *      msg: what(), string err() (numeric err())
 */
static std::string ethCLErrorHelper(const char *msg, cl::Error const &clerr) {
	std::ostringstream osstream;
	osstream << msg << ": " << clerr.what() << ": " << strClError(clerr.err())
	         << " (" << clerr.err() << ")";
	return osstream.str();
}

namespace
{

void addDefinition(string& _source, char const* _id, unsigned _value)
{
	char buf[256];
	sprintf(buf, "#define %s %uu\n", _id, _value);
	_source.insert(_source.begin(), buf, buf + strlen(buf));
}

std::vector<cl::Platform> getPlatforms()
{
	vector<cl::Platform> platforms;
	try
	{
		cl::Platform::get(&platforms);
	}
	catch(cl::Error const& err)
	{
#if defined(CL_PLATFORM_NOT_FOUND_KHR)
		if (err.err() == CL_PLATFORM_NOT_FOUND_KHR)
			cwarn << "No OpenCL platforms found";
		else
#endif
			throw err;
	}
	return platforms;
}

std::vector<cl::Device> getDevices(std::vector<cl::Platform> const& _platforms, unsigned _platformId)
{
	vector<cl::Device> devices;
	size_t platform_num = min<size_t>(_platformId, _platforms.size() - 1);
	try
	{
		_platforms[platform_num].getDevices(
			CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR,
			&devices
		);
	}
	catch (cl::Error const& err)
	{
		// if simply no devices found return empty vector
		if (err.err() != CL_DEVICE_NOT_FOUND)
			throw err;
	}
	return devices;
}

}

}
}

unsigned CLMiner::s_platformId = 0;
unsigned CLMiner::s_numInstances = 0;
int CLMiner::s_devices[16] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

CLMiner::CLMiner(FarmFace& _farm, unsigned _index):
	Miner("cl-", _farm, _index)
{}

CLMiner::~CLMiner()
{
	pause();
}

void CLMiner::report(uint64_t _nonce, WorkPackage const& _w)
{
    assert(_nonce != 0);
    // Patch notes: now has lower power usage
    WorkPackage w = work();
    // TODO: Why re-evaluating?
    Result r = EthashAux::eval(w.seed, w.header, _nonce);
    if (r.value < w.boundary)
        farm.submitProof(Solution{_nonce, r.mixHash, w.header, w.seed, w.boundary});
    else
        cwarn << "Invalid solution";
}

namespace
{
uint64_t randomNonce()
{
	static std::mt19937_64 s_gen(std::random_device{}());
	return std::uniform_int_distribution<uint64_t>{}(s_gen);
}
}

void CLMiner::workLoop()
{
	// Memory for zero-ing buffers. Cannot be static because crashes on macOS.
	uint32_t const c_zero = 0;

	uint64_t startNonce = 0;

	// The work package currently processed by GPU.
	WorkPackage current;
	current.header = h256{1u};
	current.seed = h256{1u};

	try
	{
		while (true)
		{
			const WorkPackage w = work();

			if (current.header != w.header)
			{
				// New work received. Update GPU data.
				auto localSwitchStart = std::chrono::high_resolution_clock::now();

				if (!w)
				{
					cllog << "No work. Pause for 3 s.";
					std::this_thread::sleep_for(std::chrono::seconds(3));
					continue;
				}

				cllog << "New work: header" << w.header << "target" << w.boundary.hex();

				if (current.seed != w.seed)
				{
					if (s_dagLoadMode == DAG_LOAD_MODE_SEQUENTIAL)
					{
						while (s_dagLoadIndex < index)
							this_thread::sleep_for(chrono::seconds(1));
						++s_dagLoadIndex;
					}

					cllog << "New seed" << w.seed;
					init(w.seed);
				}

				// Upper 64 bits of the boundary.
				const uint64_t target = (uint64_t)(u64)((u256)w.boundary >> 192);
				assert(target > 0);

				// Update header constant buffer.
				m_queue.enqueueWriteBuffer(m_header, CL_FALSE, 0, w.header.size, w.header.data());
				m_queue.enqueueWriteBuffer(m_searchBuffer, CL_FALSE, 0, sizeof(c_zero), &c_zero);

				m_searchKernel.setArg(0, m_searchBuffer);  // Supply output buffer to kernel.
				m_searchKernel.setArg(4, target);

				if (m_useAsmKernel) {
					m_asmSearchKernel.setArg(m_kernelArgs.m_searchBufferArg, m_searchBuffer);
					m_asmSearchKernel.setArg(m_kernelArgs.m_targetArg, target);
				}

				// FIXME: This logic should be move out of here.
				if (w.exSizeBits >= 0)
					startNonce = w.startNonce | ((uint64_t)index << (64 - 4 - w.exSizeBits)); // This can support up to 16 devices.
				else
					startNonce = randomNonce();

				current = w;
				auto switchEnd = std::chrono::high_resolution_clock::now();
				auto globalSwitchTime = std::chrono::duration_cast<std::chrono::milliseconds>(switchEnd - workSwitchStart).count();
				auto localSwitchTime = std::chrono::duration_cast<std::chrono::microseconds>(switchEnd - localSwitchStart).count();
				cllog << "Switch time" << globalSwitchTime << "ms /" << localSwitchTime << "us";
			}

			// Read results.
			// TODO: could use pinned host pointer instead.
			uint32_t results[c_maxSearchResults + 1];
			m_queue.enqueueReadBuffer(m_searchBuffer, CL_TRUE, 0, sizeof(results), &results);

			uint64_t nonce = 0;
			if (results[0] > 0)
			{
				// Ignore results except the first one.
				nonce = startNonce + results[1];
				// Reset search buffer if any solution found.
				m_queue.enqueueWriteBuffer(m_searchBuffer, CL_FALSE, 0, sizeof(c_zero), &c_zero);
			}

			// Increase start nonce for following kernel execution.
			startNonce += m_globalWorkSize;

			// Run the kernel.
			if (m_useAsmKernel) {
				m_asmSearchKernel.setArg(m_kernelArgs.m_startNonceArg, startNonce);
				m_queue.enqueueNDRangeKernel(m_asmSearchKernel, cl::NullRange, m_globalWorkSize, m_workgroupSize);
			}
			else {
				m_searchKernel.setArg(3, startNonce);
				m_queue.enqueueNDRangeKernel(m_searchKernel, cl::NullRange, m_globalWorkSize, m_workgroupSize);
			}

			// Report results while the kernel is running.
			// It takes some time because ethash must be re-evaluated on CPU.
			if (nonce != 0)
				report(nonce, current);

			// Report hash count
			addHashCount(m_globalWorkSize);

			// Check if we should stop.
			if (shouldStop())
			{
				// Make sure the last buffer write has finished --
				// it reads local variable.
				m_queue.finish();
				break;
			}
		}
	}
	catch (cl::Error const& _e)
	{
		cwarn << ethCLErrorHelper("OpenCL Error", _e);
	}
}

unsigned CLMiner::getNumDevices()
{
	vector<cl::Platform> platforms = getPlatforms();
	if (platforms.empty())
		return 0;

	vector<cl::Device> devices = getDevices(platforms, s_platformId);
	if (devices.empty())
	{
		cwarn << "No OpenCL devices found.";
		return 0;
	}
	return devices.size();
}

void CLMiner::listDevices()
{
	string outString ="\nListing OpenCL devices.\nFORMAT: [platformID] [deviceID] deviceName\n";
	unsigned int i = 0;

	vector<cl::Platform> platforms = getPlatforms();
	if (platforms.empty())
		return;
	for (unsigned j = 0; j < platforms.size(); ++j)
	{
		i = 0;
		vector<cl::Device> devices = getDevices(platforms, j);
		for (auto const& device: devices)
		{
			outString += "[" + to_string(j) + "] [" + to_string(i) + "] " + device.getInfo<CL_DEVICE_NAME>() + "\n";
			outString += "\tCL_DEVICE_TYPE: ";
			switch (device.getInfo<CL_DEVICE_TYPE>())
			{
			case CL_DEVICE_TYPE_CPU:
				outString += "CPU\n";
				break;
			case CL_DEVICE_TYPE_GPU:
				outString += "GPU\n";
				break;
			case CL_DEVICE_TYPE_ACCELERATOR:
				outString += "ACCELERATOR\n";
				break;
			default:
				outString += "DEFAULT\n";
				break;
			}
			outString += "\tCL_DEVICE_GLOBAL_MEM_SIZE: " + to_string(device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>()) + "\n";
			outString += "\tCL_DEVICE_MAX_MEM_ALLOC_SIZE: " + to_string(device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>()) + "\n";
			outString += "\tCL_DEVICE_MAX_WORK_GROUP_SIZE: " + to_string(device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>()) + "\n";
			++i;
		}
	}
	std::cout << outString;
}

bool CLMiner::configureGPU(
	unsigned _localWorkSize,
	unsigned _globalWorkSizeMultiplier,
	unsigned _platformId,
	uint64_t _currentBlock,
	unsigned _dagLoadMode,
	unsigned _dagCreateDevice
)
{
	s_dagLoadMode = _dagLoadMode;
	s_dagCreateDevice = _dagCreateDevice;

	s_platformId = _platformId;

	_localWorkSize = ((_localWorkSize + 7) / 8) * 8;
	s_workgroupSize = _localWorkSize;
	s_initialGlobalWorkSize = _globalWorkSizeMultiplier * _localWorkSize;

	uint64_t dagSize = ethash_get_datasize(_currentBlock);

	vector<cl::Platform> platforms = getPlatforms();
	if (platforms.empty())
		return false;
	if (_platformId >= platforms.size())
		return false;

	vector<cl::Device> devices = getDevices(platforms, _platformId);
	for (auto const& device: devices)
	{
		cl_ulong result = 0;
		device.getInfo(CL_DEVICE_GLOBAL_MEM_SIZE, &result);
		if (result >= dagSize)
		{
			cnote <<
				"Found suitable OpenCL device [" << device.getInfo<CL_DEVICE_NAME>()
												 << "] with " << result << " bytes of GPU memory";
			return true;
		}

		cnote <<
			"OpenCL device " << device.getInfo<CL_DEVICE_NAME>()
							 << " has insufficient GPU memory." << result <<
							 " bytes of memory found < " << dagSize << " bytes of memory required";
	}

	cout << "No GPU device with sufficient memory was found. Can't GPU mine. Remove the -G argument" << endl;
	return false;
}

HwMonitor CLMiner::hwmon()
{
	HwMonitor hw;
	unsigned int tempC = 0, fanpcnt = 0;
	if (nvmlh) {
		wrap_nvml_get_tempC(nvmlh, index, &tempC);
		wrap_nvml_get_fanpcnt(nvmlh, index, &fanpcnt);
	}
	if (adlh) {
		wrap_adl_get_tempC(adlh, index, &tempC);
		wrap_adl_get_fanpcnt(adlh, index, &fanpcnt);
	}
#if defined(__linux)
	if (sysfsh) {
		wrap_amdsysfs_get_tempC(sysfsh, index, &tempC);
		wrap_amdsysfs_get_fanpcnt(sysfsh, index, &fanpcnt);
	}
#endif
	hw.tempC = tempC;
	hw.fanP = fanpcnt;
	return hw;
}

bool CLMiner::loadBinaryKernel(string platform, cl::Device device, uint32_t dagSize128, uint32_t lightSize64, int platformId, int computeCapability, char *options)
{
	string device_name = device.getInfo<CL_DEVICE_NAME>();
	std::ifstream kernel_list("kernels.json");

	Json::Reader json_reader;
	Json::Value root;

	if (!kernel_list.good()) return false;
	if (!json_reader.parse(kernel_list, root)){
		kernel_list.close();
		cllog << "Parse error in kernel list!";
		return false;
	}

	kernel_list.close();

	for (auto itr = root.begin(); itr != root.end(); itr++)
	{
		auto key = itr.key();
		cllog << key;

		string dkey = key.asString();
		   if(dkey == device_name) {
			Json::Value droot = root[dkey];
			std::ifstream kernel_file; 

			std::vector<std::string> kparams = {
				"path", "binary", "kernel_name",
				"max_solutions", "returns_mix", "args"
			};

			std::vector<string> args = { 
				"searchBuffer", "header", "dag", 
				"startNonce", "target", "isolate", 
				"dagSize" 
			};

			/* verify all kernel parameters */
			for (auto p : kparams) {
				if (!droot.isMember(p)) {
					cllog << "Kernel definition" << dkey << "missing key" << p << "\"!";
					return false;
				}
			}
			for (auto p : args) {
				if (!droot["args"].isMember(p)) {
					cllog << "Kernel definition" << dkey << "missing argument key" << p << "!";
					return false;
				}
			}

			/* If we have a text kernel, we don't need dag size, but if it's binary, it NEEDS to be fed in*/
			if (!droot["args"].isMember("dagSize") && root[dkey]["binary"].asBool()) {
				cllog << "Kernel for " << device_name << " is a binary, but doesn't take dagSize argument! Bad kernels.json";
				return false;
			}

			/* Claymore's kernels need both of these */
			if (droot["args"].isMember("factorExp") != droot["args"].isMember("factorDenom")) {
				return false;
			}

			/* Start loading the kernel */
			kernel_file.open(
				root[dkey]["path"].asString(),
				ios::in | ios::binary
			);

			if (!kernel_file.good()) {
				cwarn << "Couldn't load kernel binary: " << root[dkey]["path"].asString();
				return false;
			}

			/* if it's a binary kernel */
			if (root[dkey]["binary"].asBool()) {
				vector<unsigned char> bin_data;

				kernel_file.unsetf(std::ios::skipws);
				bin_data.insert(bin_data.begin(),
					std::istream_iterator<unsigned char>(kernel_file),
					std::istream_iterator<unsigned char>());

				/* Setup the program */
				cl::Program::Binaries blobs({bin_data});
				cl::Program program(m_context, { device }, blobs);
				try
				{
					program.build({ device }, options);
					cllog << "Build info success:" << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
					m_asmSearchKernel = cl::Kernel(program, droot["kernel_name"].asString().c_str());
				}
				catch (cl::Error const&)
				{
					cwarn << "Build info:" << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
					return false;
				}

			}
			else {
				std::string kernel_ascii((std::istreambuf_iterator<char>(kernel_file)),
										  std::istreambuf_iterator<char>());

				addDefinition(kernel_ascii, "GROUP_SIZE", m_workgroupSize);
				addDefinition(kernel_ascii, "DAG_SIZE", dagSize128);
				addDefinition(kernel_ascii, "LIGHT_SIZE", lightSize64);
				addDefinition(kernel_ascii, "ACCESSES", ETHASH_ACCESSES);
				addDefinition(kernel_ascii, "MAX_OUTPUTS", c_maxSearchResults);
				addDefinition(kernel_ascii, "PLATFORM", platformId);
				addDefinition(kernel_ascii, "COMPUTE", computeCapability);
				addDefinition(kernel_ascii, "THREADS_PER_HASH", s_threadsPerHash);

				cl::Program::Sources sources{ { kernel_ascii.data(), kernel_ascii.size()} };
				cl::Program program(m_context, sources);

				try
				{
					program.build({ device }, options);
					cllog << "Build info:" << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
					m_asmSearchKernel = cl::Kernel(program, droot["kernel_name"].asCString());
				}
				catch (cl::Error const&)
				{
					cwarn << "Build failed!";
					cwarn << "Build info:" << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
					return false;
				}

			}

			/* Load where each kernel param should be slotted into */
			if (droot["args"].isMember("factorExp")) {
				m_kernelArgs.m_factor1Arg = root[dkey]["args"]["factorExp"].asInt();
				m_kernelArgs.m_factor2Arg = root[dkey]["args"]["factorDenom"].asInt();
			}
			if (droot["args"].isMember("dagSize")) {
				m_kernelArgs.m_dagSize128Arg = root[dkey]["args"]["dagSize"].asInt();
			}

			// Load all the argument parameters for the ke
			m_kernelArgs.m_searchBufferArg = root[dkey]["args"]["searchBuffer"].asUInt();
			m_kernelArgs.m_headerArg       = root[dkey]["args"]["header"].asUInt();
			m_kernelArgs.m_dagArg          = root[dkey]["args"]["dag"].asUInt();
			m_kernelArgs.m_startNonceArg   = root[dkey]["args"]["startNonce"].asUInt();
			m_kernelArgs.m_targetArg       = root[dkey]["args"]["target"].asUInt();
			m_kernelArgs.m_isolateArg      = root[dkey]["args"]["isolate"].asUInt();

			cllog << "Arguments";
			cllog << m_kernelArgs.m_searchBufferArg;
			cllog << m_kernelArgs.m_headerArg;
			cllog << m_kernelArgs.m_dagArg;
			cllog << m_kernelArgs.m_startNonceArg;
			cllog << m_kernelArgs.m_targetArg;
			cllog << m_kernelArgs.m_isolateArg;
			cllog << m_kernelArgs.m_dagSize128Arg;

			/* set max solutions */
			m_maxSolutions                 = root[dkey]["args"]["max_solutions"].asUInt();


			cllog << "Binary kernel loaded!";
			return true;
		}
	}
	return false;
}

bool CLMiner::init(const h256& seed)
{
	EthashAux::LightType light = EthashAux::light(seed);

	// get all platforms
	try
	{
		vector<cl::Platform> platforms = getPlatforms();
		if (platforms.empty())
			return false;

		// use selected platform
		unsigned platformIdx = min<unsigned>(s_platformId, platforms.size() - 1);

		string platformName = platforms[platformIdx].getInfo<CL_PLATFORM_NAME>();
		ETHCL_LOG("Platform: " << platformName);

		int platformId = OPENCL_PLATFORM_UNKNOWN;
		if (platformName == "NVIDIA CUDA")
		{
			platformId = OPENCL_PLATFORM_NVIDIA;
			nvmlh = wrap_nvml_create();
		}
		else if (platformName == "AMD Accelerated Parallel Processing")
		{
			platformId = OPENCL_PLATFORM_AMD;
			adlh = wrap_adl_create();
#if defined(__linux)
			sysfsh = wrap_amdsysfs_create();
#endif
		}
		else if (platformName == "Clover")
		{
			platformId = OPENCL_PLATFORM_CLOVER;
		}

		// get GPU device of the default platform
		vector<cl::Device> devices = getDevices(platforms, platformIdx);
		if (devices.empty())
		{
			ETHCL_LOG("No OpenCL devices found.");
			return false;
		}

		// use selected device
		unsigned deviceId = s_devices[index] > -1 ? s_devices[index] : index;
		cl::Device& device = devices[min<unsigned>(deviceId, devices.size() - 1)];
		string device_version = device.getInfo<CL_DEVICE_VERSION>();
		ETHCL_LOG("Device:   " << device.getInfo<CL_DEVICE_NAME>() << " / " << device_version);

		string clVer = device_version.substr(7, 3);
		if (clVer == "1.0" || clVer == "1.1")
		{
			if (platformId == OPENCL_PLATFORM_CLOVER)
			{
				ETHCL_LOG("OpenCL " << clVer << " not supported, but platform Clover might work nevertheless. USE AT OWN RISK!");
			}
			else
			{
				ETHCL_LOG("OpenCL " << clVer << " not supported - minimum required version is 1.2");
				return false;
			}
		}

		char options[256];
		int computeCapability = 0;
		if (platformId == OPENCL_PLATFORM_NVIDIA) {
			cl_uint computeCapabilityMajor;
			cl_uint computeCapabilityMinor;
			clGetDeviceInfo(device(), CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV, sizeof(cl_uint), &computeCapabilityMajor, NULL);
			clGetDeviceInfo(device(), CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV, sizeof(cl_uint), &computeCapabilityMinor, NULL);

			computeCapability = computeCapabilityMajor * 10 + computeCapabilityMinor;
			int maxregs = computeCapability >= 35 ? 72 : 63;
			sprintf(options, "-cl-nv-maxrregcount=%d", maxregs);
		}
		else {
			sprintf(options, "%s", "");
		}
		// create context
		m_context = cl::Context(vector<cl::Device>(&device, &device + 1));
		m_queue = cl::CommandQueue(m_context, device);

		// make sure that global work size is evenly divisible by the local workgroup size
		m_workgroupSize = s_workgroupSize;
		m_globalWorkSize = s_initialGlobalWorkSize;
		if (m_globalWorkSize % m_workgroupSize != 0)
			m_globalWorkSize = ((m_globalWorkSize / m_workgroupSize) + 1) * m_workgroupSize;

		uint64_t dagSize = ethash_get_datasize(light->light->block_number);
		uint32_t dagSize128 = (unsigned)(dagSize / ETHASH_MIX_BYTES);
		uint32_t lightSize64 = (unsigned)(light->data().size() / sizeof(node));

		if (m_useAsmKernel) {
			if (!loadBinaryKernel(platformName, device, dagSize128, lightSize64, platformId, computeCapability, options)) {
			cllog << "Couldn't load kernel binaries, falling back to OpenCL kernel.";
				m_useAsmKernel = false;
			}
		}

		// patch source code
		// note: CLMiner_kernel is simply ethash_cl_miner_kernel.cl compiled
		// into a byte array by bin2h.cmake. There is no need to load the file by hand in runtime
		// TODO: Just use C++ raw string literal.
		string code(CLMiner_kernel, CLMiner_kernel + sizeof(CLMiner_kernel));
		addDefinition(code, "GROUP_SIZE", m_workgroupSize);
		addDefinition(code, "DAG_SIZE", dagSize128);
		addDefinition(code, "LIGHT_SIZE", lightSize64);
		addDefinition(code, "ACCESSES", ETHASH_ACCESSES);
		addDefinition(code, "MAX_OUTPUTS", c_maxSearchResults);
		addDefinition(code, "PLATFORM", platformId);
		addDefinition(code, "COMPUTE", computeCapability);
		addDefinition(code, "THREADS_PER_HASH", s_threadsPerHash);

		// create miner OpenCL program
		cl::Program::Sources sources{{code.data(), code.size()}};
		cl::Program program(m_context, sources);
		try
		{
			program.build({device}, options);
			cllog << "Build info:" << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
		}
		catch (cl::Error const&)
		{
			cwarn << "Build info:" << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
			return false;
		}

		// create buffer for dag
		try
		{
			cllog << "Creating light cache buffer, size" << light->data().size();
			m_light = cl::Buffer(m_context, CL_MEM_READ_ONLY, light->data().size());
			cllog << "Creating DAG buffer, size" << dagSize;
			m_dag = cl::Buffer(m_context, CL_MEM_READ_ONLY, dagSize);
			cllog << "Loading kernels";
			m_searchKernel = cl::Kernel(program, "ethash_search");
			m_dagKernel = cl::Kernel(program, "ethash_calculate_dag_item");
			cllog << "Writing light cache buffer";
			m_queue.enqueueWriteBuffer(m_light, CL_TRUE, 0, light->data().size(), light->data().data());
		}
		catch (cl::Error const& err)
		{
			cwarn << ethCLErrorHelper("Creating DAG buffer failed", err);
			return false;
		}
		// create buffer for header
		ETHCL_LOG("Creating buffer for header.");
		m_header = cl::Buffer(m_context, CL_MEM_READ_ONLY, 32);

		m_searchKernel.setArg(1, m_header);
		m_searchKernel.setArg(2, m_dag);
		m_searchKernel.setArg(5, ~0u);  // Pass this to stop the compiler unrolling the loops.

		if (m_useAsmKernel) {
			m_asmSearchKernel.setArg(m_kernelArgs.m_headerArg, m_header);
			m_asmSearchKernel.setArg(m_kernelArgs.m_dagArg, m_dag);
			m_asmSearchKernel.setArg(m_kernelArgs.m_isolateArg, ~0u);
			if (m_kernelArgs.m_dagSize128Arg > 0) 
				m_asmSearchKernel.setArg(m_kernelArgs.m_dagSize128Arg, dagSize128);
		}

		// create mining buffers
		ETHCL_LOG("Creating mining buffer");
		m_searchBuffer = cl::Buffer(m_context, CL_MEM_WRITE_ONLY, (c_maxSearchResults + 1) * sizeof(uint32_t));

		cllog << "Generating DAG";

		uint32_t const work = (uint32_t)(dagSize / sizeof(node));
		uint32_t fullRuns = work / m_globalWorkSize;
		uint32_t const restWork = work % m_globalWorkSize;
		if (restWork > 0) fullRuns++;

		m_dagKernel.setArg(1, m_light);
		m_dagKernel.setArg(2, m_dag);
		m_dagKernel.setArg(3, ~0u);

		for (uint32_t i = 0; i < fullRuns; i++)
		{
			m_dagKernel.setArg(0, i * m_globalWorkSize);
			m_queue.enqueueNDRangeKernel(m_dagKernel, cl::NullRange, m_globalWorkSize, m_workgroupSize);
			m_queue.finish();
			cllog << "DAG" << int(100.0f * i / fullRuns) << '%';
		}

	}
	catch (cl::Error const& err)
	{
		cwarn << ethCLErrorHelper("OpenCL init failed", err);
		return false;
	}
	return true;
}
