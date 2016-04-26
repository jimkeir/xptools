#include "WED_FileCache.h"
#include <queue>
#include <deque>
#include <sstream>
#include <fstream>
#include "FileUtils.h"
#include "GUI_Resources.h"
#include "PlatformUtils.h"
#include "AssertUtils.h"
#include "RAII_CURL_HNDL.h"
#include "curl_http.h"
#include "curl/curl.h"
#include <time.h>

//Our static-ish class-ish FileCache
//The fully qualified path to the file cache folder
string CACHE_folder;
	
class CACHE_CacheObject
{
public:
	CACHE_CacheObject() { status = WED_file_cache_status::file_not_started; }

	//Getters for all properties
	string get_url()           const { return url; }
	void   set_url(const string& new_url) { url = new_url; }

	string get_file_name()     const { return url.substr(url.find_last_of("/") + 1); }
	string get_disk_location() const { return disk_location; }
	void   set_disk_location(const string& location) { disk_location = location; }

	RAII_CURL_HNDL & get_curl_file() { return curl_file; }
	WED_file_cache_status get_status() const { return status; }
	void                  set_status(WED_file_cache_status stat) { status = stat; }

private:
	//The file url. Lives from first request to program end
	string url;

	//Real FQPN on disk, "" if non-existant. Lives from program start to program end
	string disk_location;

	//The curl_http_get_file that is associated with this cache_object
	//Info life span: Download start to download finish
	RAII_CURL_HNDL curl_file;
		
	//The status of the file's progress
	WED_file_cache_status status;

	CACHE_CacheObject(const CACHE_CacheObject& copy);
	CACHE_CacheObject & operator= (const CACHE_CacheObject& rhs);
};
	
//Default num milliseconds
static const int CACHE_COOLDOWN_START = 400;

//Number of milliseconds to wait to before to server
int CACHE_cool_down_time;

//Maps a URL with CacheObject
vector<CACHE_CacheObject *> CACHE_file_cache;

//Files the file_cache with files in the cache folder
void WED_file_cache_init()
{
	static bool cache_initialized = false;
	if(cache_initialized == true)
	{
		return;
	}
	else
	{
		//Get the cache folder
		{
			char    base[2048];
			CACHE_folder = GetApplicationPath(base,sizeof(base));
			CACHE_folder = CACHE_folder.erase(CACHE_folder.find(string("WorldEditor.exe"))) + "wed_file_cache";
		}
		CACHE_cool_down_time = 0.0;

		//Attempt to get the folder, if non-existant make it
		vector<string> files;
		int num_files = FILE_get_directory(CACHE_folder, &files, NULL);
		if(num_files == -1)
		{
			int res = FILE_make_dir(CACHE_folder.c_str());
			if(res != 0)
			{
				AssertPrintf("Could not find or make the file cache, please check if you have sufficient rights to use the folder %s", CACHE_folder.c_str());
			}
		}
		else
		{
			//Add each file in the folder to the cache
			for (int i = 0; i < files.size(); ++i)
			{
				//We always delete during shut WED_file_cache_shutdown()
				CACHE_CacheObject * obj = new CACHE_CacheObject();
				obj->set_disk_location(CACHE_folder + "\\" + files[i]);
				obj->set_status(WED_file_cache_status::file_available);
				CACHE_file_cache.push_back(obj);
			}
		}
		
		cache_initialized = true;
		return;
	}
}

//returns an error string if there is one
static string HandleNetworkError(curl_http_get_file * mCurl)
{
	int err = mCurl->get_error();
	bool bad_net = mCurl->is_net_fail();

	stringstream ss;
	ss.str() = "";
	if(err <= CURL_LAST)
	{
		string msg = curl_easy_strerror((CURLcode) err);
		ss << "Download failed: " << msg << ". (" << err << ")";
				
		if(bad_net) ss << "(Please check your internet connectivity.)";
	}
	else if(err >= 100)
	{
		//Get the string of error data
		vector<char>    errdat;
		mCurl->get_error_data(errdat);
				
		bool is_ok = !errdat.empty();
		for(vector<char>::iterator i = errdat.begin(); i != errdat.end(); ++i)
		{
			//If the charecter is not printable
			if(!isprint(*i))
			{
				is_ok = false;
				break;
			}
		}

		if(is_ok)
		{
			string errmsg = string(errdat.begin(),errdat.end());
			ss << "Error Code " << err << ": " << errmsg;
		}
		else
		{
			//Couldn't get a useful error message, displaying this instead
			ss << "Download failed due to unknown error: " << err << ".";
		}
	}
	else
	{
		ss << "Download failed due to unknown error: " << err << ".";
	}

	return ss.str();    
}

WED_file_cache_status WED_get_file_from_cache(
			const string& in_url,
			const string& in_cert,
			string& out_path,
			string& out_error)
{
	out_error = "";
	out_path = "";
	if(CACHE_cool_down_time > 0)
	{
		--CACHE_cool_down_time;
	}
	/* 
	-------------------------Method outline------------------------------------
	1. Is it on disk?
		- Set out_path, return file_available
	2. Not on disk, is it in download queue?
		- Yes it is. It's currently...
			* Experiancing an error. Set out_error, return file_error
			* Downloading as normal. return file_downloading
	3. Not on disk, not in download queue.
		- Add file if we're not cooling down, return file_downloading.
	4. Not on disk, not in download queue, not allowed to be downloaded yet
		- Set out_error to error message, return file_cache_cooling
	---------------------------------------------------------------------------
	*/

	//Search through the file_cache on disk looking to see if we've already got this url
	for (int i = 0; i < CACHE_file_cache.size(); i++)
	{
		string file_name = in_url.substr(in_url.find_last_of('/') + 1);
		if(CACHE_file_cache[i]->get_disk_location().find(file_name) != string::npos)
		{
			out_path = CACHE_file_cache[i]->get_disk_location();
			
			//While we're here fill out more information
			return WED_file_cache_status::file_available;
		}
	}

	//If not on disk, see if it is in CACHE_file_cache, downloading or having an error
	for (vector<CACHE_CacheObject* >::iterator itr = CACHE_file_cache.begin();
		 itr != CACHE_file_cache.end();
		 ++itr)
	{
		if((*itr)->get_url() == in_url)
		{
			curl_http_get_file * hndl = (*itr)->get_curl_file().get_curl_handle();
			if(hndl != NULL)
			{
				if(hndl->is_done())
				{
					if(hndl->is_ok())
					{
						//Yay! We're done!
						const vector<char>& buf = (*itr)->get_curl_file().get_JSON_BUF();
						string result(buf.begin(),buf.end());
						out_path = CACHE_folder + "\\" + (*itr)->get_file_name();
						(*itr)->set_disk_location(out_path);

						ofstream ofs(out_path);
						ofs << result << endl;
						return WED_file_cache_status::file_available;
					}
					else
					{
						//Handle whatever error we're going to have
						 out_error = HandleNetworkError(hndl);

						 //Activate cooldown if were not already counting down
						 if(CACHE_cool_down_time == 0)
						 {
							CACHE_cool_down_time = CACHE_COOLDOWN_START;
							return WED_file_cache_status::file_cache_cooling;
						 }
						 else
						 {
							 return WED_file_cache_status::file_error;
						 }
					}//end if(hndl->is_ok()
				}
				else
				{
					return WED_file_cache_status::file_downloading;
				}//end if(hndl->is_done()
			}
		}
	}

	//If it is not on disk, and not in the download_queue, and we haven't reached our download maximum
	if(CACHE_cool_down_time == 0)
	{
		CACHE_CacheObject * obj = new CACHE_CacheObject();
		obj->get_curl_file().create_HNDL(in_url, in_cert, 0);
		obj->set_url(in_url);
		obj->set_status(WED_file_cache_status::file_downloading);
		CACHE_file_cache.push_back(obj);

		return WED_file_cache_status::file_downloading;
	}
	else
	{
		return WED_file_cache_status::file_cache_cooling;
	}
}


void WED_file_cache_shutdown()
{
	/*Generate vector of files still downloading
	//vector still doing IO
	while(true)
	{
	
	vector[i]
	delete and erase here
	
	
	*/
}

#if DEV
#include <ostream>
void WED_file_cache_test()
{
	WED_file_cache_init();
	//Test finding files already on disk, not available online
	vector<string> test_files;
	//test_files.push_back(CACHE_folder + "\\ondisk1.txt");

	//Test finding files on disk, also available online
	//The american flag on the front page
	test_files.push_back("http://www.example.com/index.html");
	
	//Test finding files not on disk, not online
	//test_files.push_back("http://www.x-plane.com/thisisnotreal.txt");

	//Test finding files not on disk, online
	//test_files.push_back("http://gateway.x-plane.com/airport_metadata.csv");

	//Get Certification
	string cert;
	//GUI_GetTempResourcePath("gateway.crt", cert);
	
	int i = 0;
	while(i < test_files.size())
	{
		WED_file_cache_status status = WED_file_cache_status::file_not_started;

		while(status != WED_file_cache_status::file_available)
		{
			string in_path = test_files.at(i);
			string out_path;
			string error;
			status = WED_get_file_from_cache(in_path, cert, out_path, error);

			printf("out_path: %s error_path: %s\n", out_path.c_str(), error.c_str());
		}
		++i;
	}
}
#endif
