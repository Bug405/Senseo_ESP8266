package com.example.senseo.MyObjects;

import java.io.Serializable;

public class Settings implements Serializable {

    private String ip;

    public String getIp() {
        return ip;
    }

    public void setIp(String ip) {
        this.ip = ip;
    }
}
