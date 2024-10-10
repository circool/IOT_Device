void setSwitchState(bool state);
bool getSwitchState();
bool getDefaultSwitchState();

void publishSensorState();
void publishConfigTopic(char * topic);
bool publishConfig();

bool publishSwitchState();


void checkMqttConnection();

void checkWiFiConnection();

struct Params;

class Config;

