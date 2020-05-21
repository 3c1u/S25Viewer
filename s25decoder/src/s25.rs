use std::fs::File;
use std::io::{BufReader, Read, Seek, SeekFrom};
use std::path::Path;

use crate::utils;

const S25_MAGIC: &[u8; 4] = b"S25\0";
const S25_BYTES_PER_PIXEL: usize = 4;

pub struct S25Archive<A = File> {
    pub file: BufReader<A>,
    pub entries: Vec<Option<i32>>,
}

pub type Result<T> = std::result::Result<T, Error>;

#[derive(Debug)]
pub enum Error {
    IoError(std::io::Error),
    InvalidArchive,
    UnsupportedFileFormat,
    NoEntry,
}

impl From<std::io::Error> for Error {
    fn from(e: std::io::Error) -> Self {
        Self::IoError(e)
    }
}

impl S25Archive<File> {
    pub fn open<P: AsRef<Path>>(path: P) -> Result<Self> {
        let path = path.as_ref();

        let file = File::open(path)?;
        let mut file = BufReader::new(file);

        let mut magic_buf = [0u8; 4];
        file.read_exact(&mut magic_buf)?;
        if &magic_buf != S25_MAGIC {
            return Err(Error::InvalidArchive);
        }

        let total_entries = utils::io::read_i32(&mut file)?;

        let mut entries = vec![];

        for _ in 0..total_entries {
            let mut offset = [0u8; 4];
            file.read_exact(&mut offset)?;
            let offset = i32::from_le_bytes(offset);

            entries.push(if offset == 0 { None } else { Some(offset) });
        }

        Ok(S25Archive { file, entries })
    }
}

impl<'a> S25Archive<std::io::Cursor<&'a [u8]>> {
    pub fn from_raw_bytes<'b>(bytes: &'b [u8]) -> Result<Self>
    where
        'b: 'a,
    {
        use std::io::Cursor;

        let mut file = BufReader::new(Cursor::new(bytes));

        let mut magic_buf = [0u8; 4];
        file.read_exact(&mut magic_buf)?;
        if &magic_buf != S25_MAGIC {
            return Err(Error::InvalidArchive);
        }

        let total_entries = utils::io::read_i32(&mut file)?;

        let mut entries = vec![];

        for _ in 0..total_entries {
            let mut offset = [0u8; 4];
            file.read_exact(&mut offset)?;
            let offset = i32::from_le_bytes(offset);

            entries.push(if offset == 0 { None } else { Some(offset) });
        }

        Ok(S25Archive { file, entries })
    }
}

impl<A> S25Archive<A> {
    pub fn total_entries(&self) -> usize {
        self.entries.len()
    }

    pub fn total_layers(&self) -> usize {
        self.total_entries() / 100 + 1
    }
}

#[derive(Clone, Copy, Debug)]
pub struct S25ImageMetadata {
    pub width: i32,
    pub height: i32,
    pub offset_x: i32,
    pub offset_y: i32,
    pub incremental: bool,
    pub head: i32,
}

#[derive(Clone)]
pub struct S25Image {
    pub metadata: S25ImageMetadata,
    pub rgba_buffer: Vec<u8>,
}

impl<A> S25Archive<A>
where
    A: Read + Seek,
{
    pub fn load_image_metadata(&mut self, entry: usize) -> Result<S25ImageMetadata> {
        let offset = self
            .entries
            .get(entry)
            .copied()
            .flatten()
            .ok_or(Error::NoEntry)?;

        self.file.seek(SeekFrom::Start(offset as u64))?;

        let width = utils::io::read_i32(&mut self.file)?;
        let height = utils::io::read_i32(&mut self.file)?;
        let offset_x = utils::io::read_i32(&mut self.file)?;
        let offset_y = utils::io::read_i32(&mut self.file)?;
        let incremental = 0 != (utils::io::read_i32(&mut self.file)? as u32 & 0x80000000);

        Ok(S25ImageMetadata {
            width,
            height,
            offset_x,
            offset_y,
            incremental,
            head: offset + 0x14,
        })
    }

    pub fn load_image(&mut self, entry: usize) -> Result<S25Image> {
        let metadata = self.load_image_metadata(entry)?;
        let mut buf = vec![0u8; (metadata.width * metadata.height) as usize * S25_BYTES_PER_PIXEL];

        self.unpack(&metadata, &mut buf)?;

        Ok(S25Image {
            metadata,
            rgba_buffer: buf,
        })
    }

    fn unpack(&mut self, metadata: &S25ImageMetadata, buf: &mut [u8]) -> Result<()> {
        // データ開始位置にカーソルを移動
        self.file.seek(SeekFrom::Start(metadata.head as u64))?;

        if metadata.incremental {
            return self.unpack_incremental(metadata, buf);
        }

        // non-incrementalな画像エントリーをロードする
        let mut rows = Vec::with_capacity(metadata.height as usize);
        for _ in 0..metadata.height {
            rows.push(utils::io::read_i32(&mut self.file)? as u32);
        }

        let mut offset = 0;
        let mut decode_buf = Vec::<u8>::with_capacity(metadata.width as usize);

        // すべての行を走査してデコードしていく
        for row_offset in rows {
            self.file.seek(SeekFrom::Start(row_offset as u64))?;
            let row_length = utils::io::read_i16(&mut self.file)? as u16;

            let row_length = if row_offset & 0x01 != 0 {
                self.file.read_exact(&mut [0u8])?; // 1バイトだけ読み飛ばす
                row_length & (!0x01)
            } else {
                row_length
            };

            decode_buf.resize(row_length as usize, 0u8);
            self.file.read_exact(&mut decode_buf)?;

            self.decode_line(&decode_buf, buf, &mut offset, metadata.width);
        }

        Ok(())
    }

    fn decode_line(&mut self, decode_buf: &[u8], buf: &mut [u8], offset: &mut usize, width: i32) {
        use std::convert::TryFrom;

        let mut decode_counter = 0usize;

        let mut count_remaining = width;

        while count_remaining > 0 && decode_counter < decode_buf.len() {
            // 偶数で正規化
            decode_counter += decode_counter & 0x01;

            let count = u16::from_le_bytes(
                *<&[u8; 2]>::try_from(&decode_buf[decode_counter..][..2]).unwrap(),
            );
            decode_counter += 2;

            let (method, skip) = (count >> 13, (count >> 11) & 0x03);
            decode_counter += skip as usize;

            let count = {
                let count = count & 0x7ff;
                if count == 0 {
                    // 拡張カウント
                    let new_count = i32::from_le_bytes(
                        *<&[u8; 4]>::try_from(&decode_buf[decode_counter..][..4]).unwrap(),
                    );
                    decode_counter += 4;
                    new_count
                } else {
                    count as i32
                }
                .min(count_remaining)
            };

            count_remaining -= count;

            match method {
                2 => {
                    // BGR
                    for _ in 0..count {
                        if buf.len() < (*offset + 4) || decode_buf.len() <= (decode_counter + 2) {
                            break;
                        }

                        buf[*offset] = decode_buf[decode_counter + 2];
                        buf[*offset + 1] = decode_buf[decode_counter + 1];
                        buf[*offset + 2] = decode_buf[decode_counter];
                        buf[*offset + 3] = 0xff;

                        decode_counter += 3;
                        *offset += 4;
                    }
                }
                3 => {
                    // BGR fill
                    if let [b, g, r] = decode_buf[decode_counter..][..3] {
                        decode_counter += 3;

                        for _ in 0..count {
                            if buf.len() < (*offset + 4) {
                                break;
                            }

                            buf[*offset] = r;
                            buf[*offset + 1] = g;
                            buf[*offset + 2] = b;
                            buf[*offset + 3] = 0xff;

                            *offset += 4;
                        }
                    } else {
                        unreachable!();
                    }
                }
                4 => {
                    // ABGR
                    for _ in 0..count {
                        if buf.len() < (*offset + 4) {
                            break;
                        }

                        buf[*offset] = decode_buf[decode_counter + 3];
                        buf[*offset + 1] = decode_buf[decode_counter + 2];
                        buf[*offset + 2] = decode_buf[decode_counter + 1];
                        buf[*offset + 3] = decode_buf[decode_counter + 0];

                        decode_counter += 4;
                        *offset += 4;
                    }
                }
                5 => {
                    // ABGR fill
                    if let [a, b, g, r] = decode_buf[decode_counter..][..4] {
                        decode_counter += 4;

                        for _ in 0..count {
                            if buf.len() < (*offset + 4) {
                                break;
                            }

                            buf[*offset] = r;
                            buf[*offset + 1] = g;
                            buf[*offset + 2] = b;
                            buf[*offset + 3] = a;
                            *offset += 4;
                        }
                    } else {
                        unreachable!();
                    }
                }
                _ => {
                    if count < 0 {
                        *offset -= (-count) as usize * 4;
                    } else {
                        *offset += count as usize * 4;
                    }
                }
            }
        }
    }

    // incremental S25
    fn unpack_incremental(&mut self, metadata: &S25ImageMetadata, buf: &mut [u8]) -> Result<()> {
        let _ = metadata;
        let _ = buf;

        Err(Error::UnsupportedFileFormat)
    }

    // fn read_line(&mut self) {}
}
