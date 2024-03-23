use proc_macro::TokenStream;
use quote::quote;
// use syn;
// use proc_macro2;

fn type_to_msg_id(input: &str) -> proc_macro2::TokenStream {
    let input_bytes = input.as_bytes();
    let total_len = (input_bytes.len() + 8) & !7; // Adding the null term
    let null_len = total_len - input_bytes.len();
    let null_bytes = vec![0u8; null_len];
    // println!("{:?}", input_bytes);
    let expanded = quote! {
        [ #(#input_bytes),* , #(#null_bytes),* , data @ ..]
    };
    expanded.into()
}

#[proc_macro_derive(RecvEnumTrait)]
pub fn derive_recv_enum_trait(input: TokenStream) -> TokenStream {
    let syn_item: syn::DeriveInput = syn::parse(input).unwrap();

    let enum_name = syn_item.ident;
    let generics = syn_item.generics;

    let variant = match syn_item.data {
        syn::Data::Enum(enum_item) => {
            enum_item.variants.into_iter().map(|v| v.ident).collect::<Vec<_>>()
        }
        _ => panic!("AllVariants only works on enums"),
    };

    let pattern = variant.iter().map(|v| type_to_msg_id(&v.to_string())).collect::<Vec<_>>();

    let expanded = quote! {
        impl #generics RecvEnumTrait #generics for #enum_name #generics {
            fn from_recv_bytes<const N: usize>(recv_box: &'a mut RecvBox<N>) -> Option<#enum_name <'a>> {
                match &mut recv_box.recv_buf[..] {
                    #(
                        // Split recv_buf into two halfs: First Half is Pattern + Struct, Second Half is Attach Arrays
                        #pattern => Some(Self::#variant(#variant::from_recv_bytes(data))),
                    )*
                    _ => None,
                }
            }
        }
    };

    // println!("{:?}", expanded.to_string());
    expanded.into()
}

#[proc_macro_derive(MsgTrait)]
pub fn derive_msg_trait(input: TokenStream) -> TokenStream {
    let syn_item: syn::DeriveInput = syn::parse(input).unwrap();

    let struct_name = syn_item.ident;
    let struct_name_str = struct_name.to_string();
    let generics = syn_item.generics;

    let struct_name_str_total_len = (struct_name_str.len() + 8) & !7; // Adding the null term

    let fields = match syn_item.data {
        syn::Data::Struct(struct_item) => {
            struct_item.fields.into_iter().collect::<Vec<_>>()
        }
        _ => panic!("MsgTrait only works on structs"),
    };

    let fields_filtered = fields
        .iter()
        .filter(|field| {
            match field.ty {
                syn::Type::Path(syn::TypePath{path: ref type_path, ..}) => {
                    let last_seg = type_path.segments.last().unwrap();
                    last_seg.ident == "AttachedArray"
                }
                _ => false
            }
        })
        .collect::<Vec<_>>();

    let fields_filtered_ident = fields_filtered
        .iter()
        .map(|field| field.ident.clone())
        .collect::<Vec<_>>();

    // println!("{:?}", fields);
    // println!("{:?}", fields_filtered_ident);

    let expanded = 
        if fields_filtered_ident.len() == 0 {
            quote! {
                impl<'a> MsgTrait<'a> for #struct_name #generics {
                    fn type_name() -> &'static str {
                        &#struct_name_str
                    }
                }
            }
        } else {
            quote! {
                impl<'a> MsgTrait<'a> for #struct_name #generics {
                    fn type_name() -> &'static str {
                        &#struct_name_str
                    }
                    
                    fn from_recv_bytes(buf: &'a mut [u8]) -> &'a mut Self {
                        let (obj_bytes, attached_buf) = buf.split_at_mut(core::mem::size_of::<Self>());

                        let obj = unsafe { &mut *(obj_bytes.as_mut_ptr() as *mut Self) };

                        // println!("RecvEnum::from_recv_bytes {:p} {}", obj, #struct_name_str_total_len);

                        let mut array_fields = [ #(&mut obj.#fields_filtered_ident)* ];
                        
                        AttachedArray::from_recv_bytes(
                            &mut array_fields,
                            attached_buf,
                            #struct_name_str_total_len + core::mem::size_of::<Self>()
                        );

                        obj
                    }
                }
            }
        };

    expanded.into()
}
