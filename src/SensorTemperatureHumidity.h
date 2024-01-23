// Temperature humidity sensor
class Sensor {
	private:
		double temperature;
		double humidity;
		int lastRead;
	public:
	double getTemperature();
	double getHumidity();

}