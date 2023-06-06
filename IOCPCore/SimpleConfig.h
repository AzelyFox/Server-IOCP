#pragma once

#include "Core.h"

namespace azely
{
	/**
	 * \brief 파일로부터 간단히 key value를 읽어오는 클래스
	 */
	class SimpleConfig
	{

	public:
		/**
		 * \param filePath 읽어올 파일 경로
		 */
		SimpleConfig(const string &filePath);
		virtual ~SimpleConfig();

		/**
		 * \brief 파일이 정상적으로 읽어와 졌는지를 리턴합니다
		 * \return 읽어온 설정값이 있는지 여부
		 */
		BOOL	IsConfigLoaded() const;

		/**
		 * \brief key 에 따른 value가 'T나' 't' 로 시작한다면 TRUE로 판별합니다
		 * \param key 읽어올 키
		 * \param outValue [out] 읽어온 값
		 * \return 읽기에 성공했는지 여부
		 */
		BOOL	GetBoolean(const string &key, BOOL *outValue) const;

		/**
		 * \brief key 에 따른 value를 리턴합니다
		 * \param key 읽어올 키
		 * \param outString [out] 읽어온 값
		 * \return 읽기에 성공했는지 여부
		 */
		BOOL	GetString(const string &key, string *outString) const;

		/**
		 * \brief key 에 따른 value를 int 형식으로 변환하여 리턴합니다
		 * \param key 읽어올 키
		 * \param outInt [out] 읽어온 값
		 * \return 읽기에 성공했는지 여부
		 */
		BOOL	GetInt(const string &key, INT32 *outInt) const;

		/**
		 * \brief key 에 따른 value를 double 형식으로 변환하여 리턴합니다
		 * \param key 읽어올 키
		 * \param outDouble [out] 읽어온 값
		 * \return 읽기에 성공했는지 여부
		 */
		BOOL	GetDouble(const string &key, DOUBLE *outDouble) const;

		/**
		 * \brief key 에 따른 value를 char 형식으로 변환하여 리턴합니다
		 * \param key 읽어올 키
		 * \param outChar [out] 읽어온 값
		 * \return 읽기에 성공했는지 여부
		 */
		BOOL	GetChar(const string &key, CHAR *outChar) const;
		
	private:

		unordered_map<string, string> settingMap;

	};



}