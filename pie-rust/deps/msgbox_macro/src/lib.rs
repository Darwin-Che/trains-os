use proc_macro::TokenStream;
use quote::quote;
use syn;
use proc_macro2;

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

    let variants = match syn_item.data {
        syn::Data::Enum(enum_item) => {
            enum_item.variants.into_iter().map(|v| v.ident).collect::<Vec<_>>()
        }
        _ => panic!("AllVariants only works on enums"),
    };

    // println!("{:?}", variants);
    let patterns = variants.iter().map(|v| type_to_msg_id(&v.to_string())).collect::<Vec<_>>();
    // for p in patterns.iter() {
    //     println!("{:?}", p.to_string());
    // }

    let expanded = quote! {
        impl #generics RecvEnumTrait #generics for #enum_name #generics {
            fn from_recv_bytes<const N: usize>(recv_box: &'a mut RecvBox<N>) -> Option<#enum_name <'a>> {
                #(
                    if let #patterns = &mut recv_box.recv_buf[..] {
                        return Some(Self::#variants(unsafe { &mut *(data.as_mut_ptr() as *mut #variants) }));
                    }
                )*
                return None;
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

    let expanded = quote! {
        impl<'a> MsgTrait<'a> for #struct_name #generics {
            fn type_name() -> &'static str {
                &#struct_name_str
            }

            fn resolve_attached_array_ref(&'a mut self, buf: &'a mut [u8]) {
                #(
                    self.#fields_filtered_ident.calc_array(buf);
                )*
            }
        }
    };
    expanded.into()
}
