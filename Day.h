class Day {
		private:
			bool _dayActive = false;
			bool _mistActive = false;
			bool _sprayActive = false;
			uint8_t _startTime_H = 0;
			uint8_t _startTime_M = 0;
			uint8_t	_mistDuration = 0;
			uint8_t _sprayDuration = 0;
			uint8_t _offSet_temp = 0;
			uint8_t _offSet_humid = 0;
		public:
			/******* Constructors *******/
			Day();
			Day(bool, bool, bool, uint8_t, uint8_t, uint8_t, uint8_t);
			/********** Setters *********/
			void setDayActive(bool);
			void setMistActive(bool);
			void setSprayActive(bool);
			void setStartTime_H(uint8_t);
			void setStartTime_M(uint8_t);
			void setMistDuration(uint8_t);
			void setSprayDuration(uint8_t);
			void setOffSet_Temp(uint8_t);
			void setOffSet_humid(uint8_t);
			void setAll(bool, bool, bool, uint8_t, uint8_t, uint8_t, uint8_t);
			/********** Getters *********/
			bool getDayActive();
			bool getMistActive();
			bool getSprayActive();
			uint8_t getStartTime_H();
			uint8_t getStartTime_M();
			uint8_t getMistDuration();
			uint8_t getSprayDuration();
			uint8_t getOffSet_Temp();
			uint8_t getOffSet_humid();
			String getData();
	};