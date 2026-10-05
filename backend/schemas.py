from pydantic import BaseModel


class SensorDataCreate(BaseModel):
    device_id: str

    temperature: float | None = None

    humidity: float | None = None

    mq2: int | None = None

    flame: bool = False

    pir: bool = False


class EventCreate(BaseModel):
    device_id: str

    event: str

    severity: str | None = None

    message: str | None = None