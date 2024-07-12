use crate::log;
use heapless::Vec;
use heapless::FnvIndexMap;

type ErrorCode = u8;
pub type HandleId = u16;

#[derive(Debug, Copy, Clone, Eq, PartialEq)]
pub enum GattUuid {
    u16(u16),
    u128(u128),
}

impl core::fmt::Display for GattUuid {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        match *self {
            GattUuid::u16(x) => {
                write!(f, "UUID-0x{:04x}", x)
            },
            GattUuid::u128(x) => {
                write!(f, "UUID-0x{:32x}", x)
            } 
        }
    }
}

impl GattUuid {
    pub fn to_vec(&self) -> Vec<u8, 16> {
        match *self {
            GattUuid::u16(x) => {
                Vec::from_slice(&x.to_le_bytes()).unwrap()
            },
            GattUuid::u128(x) => {
                Vec::from_slice(&x.to_le_bytes()).unwrap()
            }
        }
    }
}

impl Into<GattUuid> for u16 {
    fn into(self) -> GattUuid {
        GattUuid::u16(self)
    }
}

impl Into<GattUuid> for u128 {
    fn into(self) -> GattUuid {
        GattUuid::u128(self)
    }
}

impl<'a> TryFrom<&'a [u8]> for GattUuid {
    type Error = &'static str;

    fn try_from(value: &'a [u8]) -> Result<Self, Self::Error> {
        if let Ok(x) = TryInto::<[u8; 2]>::try_into(value) {
            Ok(Self::u16(u16::from_le_bytes(x)))
        } else if let Ok(x) = TryInto::<[u8; 16]>::try_into(value) {
            Ok(Self::u128(u128::from_le_bytes(x)))
        } else {
            Err("Value Length is wrong")
        }
    }
}

pub const CHARAC_PROPERTY_READ: u8 = 0x02;
pub const CHARAC_PROPERTY_WRITE_WORESP: u8 = 0x04;
pub const CHARAC_PROPERTY_WRITE: u8 = 0x08;
pub const CHARAC_PROPERTY_NOTIFY: u8 = 0x10;
pub const CHARAC_PROPERTY_INDICATE: u8 = 0x20;

pub const CHARAC_CLIENT_CONFIG_NOTIFY: u8 = 0x01;
pub const CHARAC_CLIENT_CONFIG_INDICATE: u8 = 0x02;

const GATT_ATTR_MAX: usize = 256;

#[derive(Debug)]
pub struct GattBuilderService {
    pub service_uuid: GattUuid,
}

#[derive(Debug)]
pub struct GattBuilderCharac {
    pub name: Option<&'static str>,
    pub charac_uuid: GattUuid,
    pub property: u8,
    pub init_val: Option<Vec<u8, 400>>,
}

#[derive(Debug)]
pub struct GattAtt {
    pub handle: HandleId,
    pub att_type: GattUuid,
    pub att_val: Vec<u8, 400>,
    // permission
}

#[derive(Debug, Copy, Clone)]
pub struct GattCharac {
    pub name: &'static str,
    pub handle: HandleId,
    pub value_handle: HandleId,
    pub client_config_handle: Option<HandleId>,
}

#[derive(Debug)]
pub struct Gatt {
    charac_map: FnvIndexMap<&'static str, GattCharac, GATT_ATTR_MAX>, // To the Character Handle
    value_handle_map: FnvIndexMap<HandleId, &'static str, GATT_ATTR_MAX>,
    attr_vec: Vec<GattAtt, GATT_ATTR_MAX>,
    service_range: Vec<(HandleId, HandleId), GATT_ATTR_MAX>, // (Start, End)
}

impl Gatt {
    pub fn print_attr_vec(&self) {
        for i in 0..self.attr_vec.len() {
            if let Some(name) = self.value_handle_map.get(&(i as u16)) {
                log!("[GATT PRINT] ATT_{}_{} = {:?}", i, name, &self.attr_vec[i]);
            } else {
                log!("[GATT PRINT] ATT_{} = {:?}", i, &self.attr_vec[i]);
            }
        }
    }

    pub fn new(builder: &[(GattBuilderService, &[GattBuilderCharac])]) -> Self {
        let mut init = Self {
            charac_map: FnvIndexMap::new(),
            value_handle_map: FnvIndexMap::new(),
            attr_vec: Vec::new(),
            service_range: Vec::new(),
        };

        for (service, charac_list) in builder {
            let start_handle = init.attr_vec.len() as u16;
            init.attr_vec.push(
                GattAtt {
                    handle: start_handle,
                    att_type: 0x2800u16.into(),
                    att_val: Vec::from_slice(&service.service_uuid.to_vec()).unwrap(),
                }).unwrap();
            for charac in *charac_list {
                let charac_handle = init.attr_vec.len() as u16;
                let charac_value_handle = charac_handle + 1;
                // Define charac
                let mut charac_att_bytes = Vec::from_slice(&[charac.property, (charac_value_handle & 0xff) as u8, (charac_value_handle >> 8) as u8]).unwrap();
                charac_att_bytes.extend_from_slice(&charac.charac_uuid.to_vec()).unwrap();
                init.attr_vec.push(
                    GattAtt {
                        handle: charac_handle,
                        att_type: 0x2803u16.into(),
                        att_val: charac_att_bytes,
                    }).unwrap();
                // Define charac value
                init.attr_vec.push(
                    GattAtt {
                        handle: charac_value_handle,
                        att_type: charac.charac_uuid,
                        att_val: if let Some(init_val) = &charac.init_val {Vec::from_slice(&init_val).unwrap()} else {Vec::new()},
                    }).unwrap();
                // Define description if applicable
                if let Some(name) = charac.name {
                    init.attr_vec.push(
                        GattAtt {
                            handle: init.attr_vec.len() as u16,
                            att_type: 0x2901u16.into(),
                            att_val: Vec::from_slice(name.as_bytes()).unwrap(),
                        }).unwrap();
                }
                // Define client config if applicable
                let mut client_config_handle = None;
                if charac.property & (CHARAC_PROPERTY_NOTIFY | CHARAC_PROPERTY_INDICATE) != 0 {
                    let handle = init.attr_vec.len() as u16;
                    init.attr_vec.push(
                        GattAtt {
                            handle: handle,
                            att_type: 0x2902u16.into(),
                            att_val: Vec::from_slice(&[0]).unwrap(),
                        }).unwrap();
                    client_config_handle = Some(handle);
                }
                // Insert in lookup maps
                if let Some(name) = charac.name {
                    init.charac_map.insert(name, GattCharac {
                        name: name,
                        handle: charac_handle,
                        value_handle: charac_value_handle,
                        client_config_handle: client_config_handle,
                    }).unwrap();
                    init.value_handle_map.insert(charac_value_handle, name).unwrap();
                }
            }
            let end_handle = init.attr_vec.len() as u16;
            init.service_range.push((start_handle, end_handle - 1)).unwrap();
        }

        init
    }

    pub fn att_read(&self, handle: HandleId) -> Result<&GattAtt, u8> {
        self.attr_vec.get(handle as usize).ok_or(0)
    }

    pub fn att_write(&mut self, handle: HandleId, bytes: &[u8]) -> Result<(), u8> {
        let attr = self.attr_vec.get_mut(handle as usize).ok_or(0)?;
        attr.att_val.clear();
        attr.att_val.extend_from_slice(bytes);
        Ok(())
    }

    pub fn gatt_read_end_of_group(&self, handle: HandleId) -> Result<HandleId, u8> {
        for (s, e) in &self.service_range {
            if *s == handle {
                return Ok(*e);
            }
        }
        Err(0)
    }

    pub fn gatt_read_by_group_type(&self, range: [HandleId; 2], attr_type: GattUuid) -> Vec<HandleId, GATT_ATTR_MAX> {
        self.gatt_read_by_type(range, attr_type)
    }

    pub fn gatt_read_by_type(&self, range: [HandleId; 2], attr_type: GattUuid) -> Vec<HandleId, GATT_ATTR_MAX> {
        let mut res = Vec::new();
        for handle_id in (range[0] as usize)..(range[1] as usize) + 1 {
            if handle_id >= self.attr_vec.len() {
                break;
            }
            let attr = &self.attr_vec[handle_id];
            if attr.att_type == attr_type {
                if res.len() != 0 && self.attr_vec[res[0] as usize].att_val.len() != attr.att_val.len() {
                    break;
                }
                // Push item
                res.push(handle_id as u16).unwrap();
            }
        }
        res
    }

    pub fn gatt_find_info(&self, range: [HandleId; 2]) -> Vec<HandleId, GATT_ATTR_MAX> {
        let mut res = Vec::new();
        for handle_id in (range[0] as usize)..(range[1] as usize) + 1 {
            if handle_id >= self.attr_vec.len() {
                break;
            }
            let attr = &self.attr_vec[handle_id];

            if res.len() != 0 && self.attr_vec[res[0] as usize].att_type.to_vec().len() != attr.att_type.to_vec().len() {
                break;
            }
            // Push item
            res.push(handle_id as u16).unwrap();
        }
        res
    }

    pub fn get_charac_by_client_config_handle(&self, client_config_handle: HandleId) -> Option<GattCharac> {
        for charac in self.charac_map.values() {
            if charac.client_config_handle == Some(client_config_handle) {
                return Some(*charac);
            }
        }
        None
    }

    // Returns (ValueHandleId, Notify OR Indicate)
    pub fn charac_by_name(&self, name: &str) -> Option<GattCharac> {
        self.charac_map.get(name).copied()
    }

    // Returns name
    pub fn name_by_value_handle(&self, value_handle_id: HandleId) -> Option<&'static str> {
        self.value_handle_map.get(&value_handle_id).copied()
    }

    pub fn clear_subscription(&mut self) {
        for charac in self.charac_map.values() {
            if let Some(client_config_handle) = charac.client_config_handle {
                self.attr_vec[client_config_handle as usize].att_val[0] = 0x0;
            } 
        }
    }

    // pub fn name_by_value_handle(&self, value_handle: HandleId) -> &'static str {

    // }
}