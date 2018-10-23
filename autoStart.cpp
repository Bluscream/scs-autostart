#ifdef _WIN32
#  define WINVER 0x0500
#  define _WIN32_WINNT 0x0500
#  include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <string>
#include <fstream>
#include <streambuf>
#include <windows.h>
#include <cstdio>
#include <vector>
#include <sstream>
#include <iterator>
#include <fstream>

#include "scssdk_telemetry.h"
#include "eurotrucks2/scssdk_eut2.h"
#include "eurotrucks2/scssdk_telemetry_eut2.h"
#include "amtrucks/scssdk_ats.h"
#include "amtrucks/scssdk_telemetry_ats.h"

#define UNUSED(x)

LPCWSTR lpFileName = L"plugins/autoStart.ini";
LPCWSTR lpSection = L"AutoStarts";
std::vector<std::string> after_sdk_warning;
std::vector<std::string> on_sdk_config;
std::vector<std::string> on_game_pause;
std::vector<std::string> on_game_resume;
std::vector<std::string> on_game_exit;
FILE *log_file = NULL;
bool output_paused = true;
bool print_header = true;
scs_timestamp_t last_timestamp = static_cast<scs_timestamp_t>(-1);

struct telemetry_state_t
{
	scs_timestamp_t timestamp;
	scs_timestamp_t raw_rendering_timestamp;
	scs_timestamp_t raw_simulation_timestamp;
	scs_timestamp_t raw_paused_simulation_timestamp;

	bool	orientation_available;
	float	heading;
	float	pitch;
	float	roll;

	float	speed;
	float	rpm;
	int	gear;

} telemetry;

scs_log_t game_log = NULL;

bool init_log(void)
{
	if (log_file) {
		return true;
	}
	log_file = fopen("plugins/autoStart.log", "wt");
	if (!log_file) {
		return false;
	}
	fprintf(log_file, "Log opened\n");
	return true;
}

void finish_log(void)
{
	if (!log_file) {
		return;
	}
	fprintf(log_file, "Log ended\n");
	fclose(log_file);
	log_file = NULL;
}

void log_print(const char *const text, ...)
{
	if (!log_file) {
		return;
	}
	va_list args;
	va_start(args, text);
	vfprintf(log_file, text, args);
	va_end(args);
}

void log_line(const char *const text, ...)
{
	if (!log_file) {
		return;
	}
	va_list args;
	va_start(args, text);
	vfprintf(log_file, text, args);
	fprintf(log_file, "\n");
	va_end(args);
}

template<typename Out>
void split(const std::string &s, const char delim, Out result) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		*(result++) = item;
	}
}

std::vector<std::string> split(const std::string &s, const char delim) {
	std::vector<std::string> elems;
	split(s, delim, std::back_inserter(elems));
	return elems;
}

template<size_t Size>
void read_split_list(wchar_t(&splitbuffer)[Size], std::vector<std::string> &out, char split_char)
{
	char outbuffer[Size];
	size_t converted;
	wcstombs_s<Size>(&converted, outbuffer, splitbuffer, Size);
	const std::string ignorestr(outbuffer, converted - 1);
	out = split(ignorestr, split_char);
}

bool file_exists(const LPCWSTR file_name)
{
	std::ifstream file(file_name);
	return file.good();
}

std::wstring s2ws(const std::string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}

void create_config()
{
	WritePrivateProfileString(lpSection, L"after_sdk_warning", L"", lpFileName); // plugins/season_changer/Season Changer.exe
	WritePrivateProfileString(lpSection, L"on_sdk_config", L"", lpFileName);
	WritePrivateProfileString(lpSection, L"on_game_pause", L"", lpFileName);
	WritePrivateProfileString(lpSection, L"on_game_resume", L"", lpFileName);
	WritePrivateProfileString(lpSection, L"on_game_exit", L"", lpFileName);
	log_line("Created config %ls\n", lpFileName);
}

void read_config()
{
	if (!file_exists(lpFileName)) {
		create_config();
	}
	wchar_t splitbuffer[4096];
	GetPrivateProfileString(lpSection, L"after_sdk_warning", L"", splitbuffer, sizeof(splitbuffer), lpFileName);
	read_split_list(splitbuffer, after_sdk_warning, ',');
	for (const auto &app : after_sdk_warning)
		log_line("after_sdk_warning[]: \"%s\"", app.c_str());
	GetPrivateProfileString(lpSection, L"on_sdk_config", L"", splitbuffer, sizeof(splitbuffer), lpFileName);
	read_split_list(splitbuffer, on_sdk_config, ',');
	for (const auto &app : on_sdk_config)
		log_line(")[]: \"%s\"", app.c_str());
	GetPrivateProfileString(lpSection, L"on_game_pause", L"", splitbuffer, sizeof(splitbuffer), lpFileName);
	read_split_list(splitbuffer, on_game_pause, ',');
	for (const auto &app : on_game_pause)
		log_line("on_game_pause[]: \"%s\"", app.c_str());
	GetPrivateProfileString(lpSection, L"on_game_resume", L"", splitbuffer, sizeof(splitbuffer), lpFileName);
	read_split_list(splitbuffer, on_game_resume, ',');
	for (const auto &app : on_game_resume)
		log_line("on_game_resume[]: \"%s\"", app.c_str());
	GetPrivateProfileString(lpSection, L"on_game_exit", L"", splitbuffer, sizeof(splitbuffer), lpFileName);
	read_split_list(splitbuffer, on_game_exit, ',');
	for (const auto &app : on_game_exit)
		log_line("on_game_exit[]: \"%s\"", app.c_str());
	log_line("Loaded config %ls\n", lpFileName);
}

void start_apps(std::vector<std::string> apps) {
	for each(std::string app in apps) {
		std::wstring stemp = s2ws(app);
		ShellExecute(NULL, NULL, stemp.c_str(), NULL, NULL, SW_HIDE);
	}
}

SCSAPI_VOID telemetry_pause(const scs_event_t event, const void *const UNUSED(event_info), const scs_context_t UNUSED(context))
{
	output_paused = (event == SCS_TELEMETRY_EVENT_paused);
	if (output_paused) {
		log_line("Telemetry paused");
		start_apps(on_game_pause);
	}
	else {
		log_line("Telemetry unpaused");
		start_apps(on_game_resume);
	}
	print_header = true;
}

SCSAPI_VOID telemetry_configuration(const scs_event_t event, const void *const event_info, const scs_context_t UNUSED(context)) {
	start_apps(on_sdk_config);
}

SCSAPI_RESULT scs_telemetry_init(const scs_u32_t version, const scs_telemetry_init_params_t *const params)
{
	if (version != SCS_TELEMETRY_VERSION_1_00) {
		return SCS_RESULT_unsupported;
	}

	const scs_telemetry_init_params_v100_t *const version_params = static_cast<const scs_telemetry_init_params_v100_t *>(params);
	if (!init_log()) {
		version_params->common.log(SCS_LOG_TYPE_error, "Unable to initialize the log file");
		return SCS_RESULT_generic_error;
	}

	log_line("Game '%s' %u.%u", version_params->common.game_id, SCS_GET_MAJOR_VERSION(version_params->common.game_version), SCS_GET_MINOR_VERSION(version_params->common.game_version));

	if (strcmp(version_params->common.game_id, SCS_GAME_ID_EUT2) != 0) {
		log_line("UNSUPPORTED GAME: SHUTTING DOWN!");
		return SCS_RESULT_generic_error;
	}

	game_log = version_params->common.log;
	game_log(SCS_LOG_TYPE_message, "Initializing autostart log example");

	memset(&telemetry, 0, sizeof(telemetry));
	print_header = true;
	last_timestamp = static_cast<scs_timestamp_t>(-1);

	output_paused = true;


	read_config();

	start_apps(after_sdk_warning);

	const bool events_registered = (version_params->register_for_event(SCS_TELEMETRY_EVENT_paused, telemetry_pause, NULL) == SCS_RESULT_ok) && (version_params->register_for_event(SCS_TELEMETRY_EVENT_started, telemetry_pause, NULL) == SCS_RESULT_ok);
	if (!events_registered) {
		version_params->common.log(SCS_LOG_TYPE_error, "Unable to register event callbacks");
		return SCS_RESULT_generic_error;
	}
	version_params->register_for_event(SCS_TELEMETRY_EVENT_configuration, telemetry_configuration, NULL);

	return SCS_RESULT_ok;
}

SCSAPI_VOID telemetry_frame_start(const scs_event_t UNUSED(event), const void *const event_info, const scs_context_t UNUSED(context)) {
}

SCSAPI_VOID telemetry_frame_end(const scs_event_t UNUSED(event), const void *const UNUSED(event_info), const scs_context_t UNUSED(context)) {
}

SCSAPI_VOID telemetry_store_orientation(const scs_string_t name, const scs_u32_t index, const scs_value_t *const value, const scs_context_t context) {
}

SCSAPI_VOID telemetry_store_float(const scs_string_t name, const scs_u32_t index, const scs_value_t *const value, const scs_context_t context) {
}

SCSAPI_VOID telemetry_store_s32(const scs_string_t name, const scs_u32_t index, const scs_value_t *const value, const scs_context_t context) {
}

SCSAPI_VOID scs_telemetry_shutdown(void)
{
	game_log = NULL;
	finish_log();
	start_apps(on_game_exit);
}

#ifdef _WIN32
BOOL APIENTRY DllMain(
	HMODULE module,
	DWORD  reason_for_call,
	LPVOID reseved
)
{
	if (reason_for_call == DLL_PROCESS_DETACH) {
		finish_log();
		start_apps(on_game_exit);
	}
	return TRUE;
}
#endif

#ifdef __linux__
void __attribute__ ((destructor)) unload(void)
{
	finish_log();
}
#endif
