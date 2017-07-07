/**
* @file Audio.cpp
*/
#include "Audio.h"
#include <xaudio2.h>
#include <vector>
#include <list>
#include <stdint.h>
#include <wrl/client.h>
#include <algorithm>

using Microsoft::WRL::ComPtr;

namespace Audio {

const uint32_t FOURCC_RIFF_TAG = MAKEFOURCC('R', 'I', 'F', 'F');
const uint32_t FOURCC_FORMAT_TAG = MAKEFOURCC('f', 'm', 't', ' ');
const uint32_t FOURCC_DATA_TAG = MAKEFOURCC('d', 'a', 't', 'a');
const uint32_t FOURCC_WAVE_FILE_TAG = MAKEFOURCC('W', 'A', 'V', 'E');
const uint32_t FOURCC_XWMA_FILE_TAG = MAKEFOURCC('X', 'W', 'M', 'A');
const uint32_t FOURCC_XWMA_DPDS = MAKEFOURCC('d', 'p', 'd', 's');

struct RIFFChunk
{
  uint32_t tag;
  uint32_t size;
};

/**
* WAV�f�[�^.
*/
struct WaveFormatInfo
{
  union U {
    WAVEFORMATEXTENSIBLE ext;
    struct ADPCMWAVEFORMAT {
      WAVEFORMATEX    wfx;
      WORD            wSamplesPerBlock;
      WORD            wNumCoef;
      ADPCMCOEFSET    coef[7];
    } adpcm;
  } u;
  size_t dataOffset; ///< �t�@�C�����̃T�E���h�f�[�^�̐擪�I�t�Z�b�g.
  size_t dataSize; ///< �T�E���h�f�[�^�̃o�C�g��.
  size_t seekOffset; ///< WXMA�V�[�N�e�[�u���̐擪�I�t�Z�b�g.
  size_t seekSize; ///< WXMA�V�[�N�e�[�u���̃o�C�g��.
};

typedef std::vector<uint8_t> BufferType;

struct ScopedHandle
{
  ScopedHandle(HANDLE h) : handle(h) {}
  ~ScopedHandle() { if (handle != INVALID_HANDLE_VALUE) { CloseHandle(handle); } }
  operator HANDLE() { return handle; }
  HANDLE handle;
};

bool Read(HANDLE hFile, void* buf, DWORD size)
{
  DWORD readSize;
  if (!ReadFile(hFile, buf, size, &readSize, nullptr) || readSize != size) {
    return false;
  }
  return true;
}

/**
* �g��WAV�t�H�[�}�b�g�f�[�^����t�H�[�}�b�g�ɑΉ�����FOURCC���擾����.
*
* @param wfext �g��WAV�t�H�[�}�b�g�f�[�^.
*
* @return �t�H�[�}�b�g�ɑΉ�����FOURCC���ʎq.
*/
uint32_t GetWaveFormatTag(const WAVEFORMATEXTENSIBLE& wfext)
{
  if (wfext.Format.wFormatTag != WAVE_FORMAT_EXTENSIBLE) {
    return wfext.Format.wFormatTag;
  }
  return wfext.SubFormat.Data1;
}

// �t�H�[�}�b�g�����擾
bool LoadWaveFile(HANDLE hFile, WaveFormatInfo& wf, std::vector<UINT32>& seekTable, std::vector<uint8_t>* source)
{
  RIFFChunk riffChunk;
  if (!Read(hFile, &riffChunk, sizeof(riffChunk))) {
    return false;
  }
  if (riffChunk.tag != FOURCC_RIFF_TAG) {
    return false;
  }

  uint32_t fourcc;
  if (!Read(hFile, &fourcc, sizeof(fourcc))) {
    return false;
  }
  if (fourcc != FOURCC_WAVE_FILE_TAG && fourcc != FOURCC_XWMA_FILE_TAG) {
    return false;
  }

  bool hasWaveFormat = false;
  bool hasData = false;
  bool hasDpds = false;
  size_t offset = 12;
  do {
    if (SetFilePointer(hFile, offset, nullptr, FILE_BEGIN) != offset) {
      return false;
    }

    RIFFChunk chunk;
    if (!Read(hFile, &chunk, sizeof(chunk))) {
      return false;
    }

    switch (chunk.tag) {
    case FOURCC_FORMAT_TAG:
      if (!Read(hFile, &wf.u, std::min(chunk.size, sizeof(WaveFormatInfo::U)))) {
        return false;
      }
      switch (GetWaveFormatTag(wf.u.ext)) {
      case WAVE_FORMAT_PCM:
        wf.u.ext.Format.cbSize = 0;
        /* FALLTHROUGH */
      case WAVE_FORMAT_IEEE_FLOAT:
      case WAVE_FORMAT_ADPCM:
        wf.seekSize = 0;
        wf.seekOffset = 0;
        hasDpds = true;
        break;
      case WAVE_FORMAT_WMAUDIO2:
      case WAVE_FORMAT_WMAUDIO3:
        break;
      default:
        // ���̃R�[�h�ŃT�|�[�g���Ȃ��t�H�[�}�b�g.
        return false;
      }
      hasWaveFormat = true;
      break;

    case FOURCC_DATA_TAG:
      wf.dataOffset = offset + sizeof(RIFFChunk);
      wf.dataSize = chunk.size;
      if (source) {
        source->resize(wf.dataSize);
        if (!Read(hFile, source->data(), wf.dataSize)) {
          return false;
        }
      }
      hasData = true;
      break;

    case FOURCC_XWMA_DPDS:
      wf.seekOffset = offset + sizeof(RIFFChunk);
      wf.seekSize = chunk.size / 4;
      seekTable.resize(wf.seekSize);
      if (!Read(hFile, seekTable.data(), wf.seekSize * 4)) {
        return false;
      }
      // XWMA��PowerPC���ڂ�XBOX360�p�ɊJ�����ꂽ���߁A�f�[�^�̓r�b�O�G���f�B�A���ɂȂ��Ă���.
      // X86�̓��g���G���f�B�A���Ȃ̂ŕϊ����Ȃ���΂Ȃ�Ȃ�.
      for (auto& e : seekTable) {
        e = _byteswap_ulong(e);
      }
      hasDpds = true;
      break;
    }
    offset += chunk.size + sizeof(RIFFChunk);
  } while (!hasWaveFormat || !hasData || !hasDpds);
  return true;
}

/**
* Sound�̎���.
*/
class SoundImpl : public Sound
{
public:
  SoundImpl() :
    state(State_Create), sourceVoice(nullptr) {}
  virtual ~SoundImpl() override {
    if (sourceVoice) {
      sourceVoice->DestroyVoice();
    }
  }
  virtual bool Play(int flags) override {
    if (!(state & State_Pausing)) {
      Stop();
      XAUDIO2_BUFFER buffer = {};
      buffer.Flags = XAUDIO2_END_OF_STREAM;
      buffer.AudioBytes = source.size();
      buffer.pAudioData = source.data();
      buffer.LoopCount = flags & Flag_Loop ? XAUDIO2_LOOP_INFINITE : XAUDIO2_NO_LOOP_REGION;
      if (seekTable.empty()) {
        if (FAILED(sourceVoice->SubmitSourceBuffer(&buffer))) {
          return false;
        }
      } else {
        const XAUDIO2_BUFFER_WMA seekInfo = { seekTable.data(), seekTable.size() };
        if (FAILED(sourceVoice->SubmitSourceBuffer(&buffer, &seekInfo))) {
          return false;
        }
      }
    }
    state = State_Playing;
    return SUCCEEDED(sourceVoice->Start());
  }
  virtual bool Pause() override {
    if (state & State_Playing) {
      state |= State_Pausing;
      return SUCCEEDED(sourceVoice->Stop());
    }
    return false;
  }
  virtual bool Seek() override {
    return true;
  }
  virtual bool Stop() override {
    if (state & State_Playing) {
      if (!(state & State_Pausing) && FAILED(sourceVoice->Stop())) {
        return false;
      }
      state = State_Stopped;
      return SUCCEEDED(sourceVoice->FlushSourceBuffers());
    }
    return false;
  }
  virtual float SetVolume(float volume) override {
    sourceVoice->SetVolume(volume);
    return volume;
  }
  virtual float SetPitch(float pitch) override {
    sourceVoice->SetFrequencyRatio(pitch);
    return pitch;
  }
  virtual int GetState() const override {
    XAUDIO2_VOICE_STATE s;
    sourceVoice->GetState(&s);
    return s.BuffersQueued ? state : (State_Stopped | State_Prepared);
  }

  int state;
  IXAudio2SourceVoice* sourceVoice;
  std::vector<uint8_t> source;
  std::vector<UINT32> seekTable;
};

/**
* Streaming Sound�̎���.
*/
class StreamSoundImpl : public Sound
{
public:
  StreamSoundImpl() = delete;
  explicit StreamSoundImpl(HANDLE h);
  virtual ~StreamSoundImpl() override;
  virtual bool Play(int flags) override;
  virtual bool Pause() override;
  virtual bool Seek() override;
  virtual bool Stop() override;
  virtual float SetVolume(float volume) override;
  virtual float SetPitch(float pitch) override;
  virtual int GetState() const override;
  bool Update();
  void SubmitBuffer();

  static const size_t BUFFER_SIZE = 0x10000;
  static const int MAX_BUFFER_COUNT = 3;

  IXAudio2SourceVoice* sourceVoice = nullptr;
  std::vector<uint8_t> buf;
  std::vector<UINT32> seekTable;
  ScopedHandle handle;
  OVERLAPPED ol;
  size_t dataSize = 0;
  size_t dataOffset = 0;
  size_t packetSize = 0;
  int state = State_Create;
  bool loop = false;
  bool hasAsyncRead = false;
  size_t currentPos = 0;
  int curBuf = 0;
};

/**
* �R���X�g���N�^.
*
* @param h �T�E���h�t�@�C���n���h��.
*/
StreamSoundImpl::StreamSoundImpl(HANDLE h) :
  sourceVoice(nullptr), handle(h), state(State_Create), loop(false), currentPos(0), curBuf(0)
{
  buf.resize(BUFFER_SIZE * MAX_BUFFER_COUNT);
}

/**
* �f�X�g���N�^.
*/
StreamSoundImpl::~StreamSoundImpl()
{
  if (sourceVoice) {
    sourceVoice->DestroyVoice();
  }
}

/**
* �Đ�.
*
* @param loop ���[�v�t���O.
*/
bool StreamSoundImpl::Play(int flags)
{
  if (!(state & State_Pausing)) {
    Stop();
  }
  state = State_Playing;
  loop = flags & Flag_Loop;
  return SUCCEEDED(sourceVoice->Start());
}

/**
* �ꎞ��~.
*/
bool StreamSoundImpl::Pause()
{
  if (state & State_Playing && !(state & State_Pausing)) {
    state |= State_Pausing;
    return SUCCEEDED(sourceVoice->Stop());
  }
  return false;
}

/**
*
*/
bool StreamSoundImpl::Seek()
{
  return true;
}

/**
* ��~.
*/
bool StreamSoundImpl::Stop()
{
  if (state & State_Playing) {
    if (!(state & State_Pausing) && FAILED(sourceVoice->Stop())) {
      return false;
    }
    state = State_Stopped;
    return SUCCEEDED(sourceVoice->FlushSourceBuffers());
  }
  return false;
}

/**
* ���ʂ�ݒ肷��.
*
* @param volume ����(0=���� 1=�����Ȃ� 1�ȏ�=���ʑ���).
*/
float StreamSoundImpl::SetVolume(float volume)
{
  sourceVoice->SetVolume(volume);
  return volume;
}

/**
* ������ݒ肷��.
*
* @param pitch �s�b�`(1����=������������ 1=�ω��Ȃ� 1�ȏ�=�������グ��).
*/
float StreamSoundImpl::SetPitch(float pitch)
{
  sourceVoice->SetFrequencyRatio(pitch);
  return pitch;
}

/**
* �Đ���Ԃ𒲂ׂ�.
*
* @return �Đ����.
*/
int StreamSoundImpl::GetState() const
{
  XAUDIO2_VOICE_STATE s;
  sourceVoice->GetState(&s);
  return s.BuffersQueued ? (state | State_Prepared) : State_Stopped;
}

/**
* �X�g���[�~���O�T�E���h�̏�Ԃ��X�V����.
*/
bool StreamSoundImpl::Update()
{
  SubmitBuffer();

  // ��x�ɓǂݍ��ރo�C�g�����v�Z����. 0�̏ꍇ�͓ǂݍ��ރf�[�^���Ȃ��̂ŉ������Ȃ�.
  const DWORD cbValid = std::min(BUFFER_SIZE, dataSize - currentPos);
  if (cbValid == 0) {
    return false;
  }
  // �S�Ẵo�b�t�@���\�[�X�{�C�X�I�u�W�F�N�g�ɓn����Ă����牽�����Ȃ�.
  XAUDIO2_VOICE_STATE state;
  sourceVoice->GetState(&state);
  if (state.BuffersQueued >= MAX_BUFFER_COUNT) {
    return true;
  }

  // �󂫃o�b�t�@�ɑ΂��Ĕ񓯊��ǂݍ��݂��J�n����.
  if (seekTable.empty()) {
    if (!ReadFileEx(handle, &buf[BUFFER_SIZE * curBuf], cbValid, &ol, nullptr)) {
      return false;
    }
  } else {
    const UINT packetCount = cbValid / packetSize;
    if (!ReadFileEx(handle, &buf[BUFFER_SIZE * curBuf], packetCount * packetSize, &ol, nullptr)) {
      return false;
    }
  }
  if (GetLastError() != ERROR_SUCCESS) {
    return false;
  }
  hasAsyncRead = true;
  return true;
}

/**
* �񓯊��ǂݍ��݂��������Ă�����A�ǂݍ��݃o�b�t�@���\�[�X�{�C�X�I�u�W�F�N�g�ɓn��.
*/
void StreamSoundImpl::SubmitBuffer()
{
  DWORD transferedBytes;
  if (!hasAsyncRead || !GetOverlappedResult(handle, &ol, &transferedBytes, FALSE)) {
    return;
  }
  XAUDIO2_BUFFER buffer = {};
  buffer.pAudioData = &buf[BUFFER_SIZE * curBuf];
  buffer.AudioBytes = transferedBytes;
  const size_t restBytes = dataSize - currentPos;
  buffer.Flags = restBytes > BUFFER_SIZE ? 0 : XAUDIO2_END_OF_STREAM;
  if (seekTable.empty()) {
    sourceVoice->SubmitSourceBuffer(&buffer, nullptr);
  } else {
    const XAUDIO2_BUFFER_WMA bufWma = {
      seekTable.data() + (currentPos / packetSize),
      transferedBytes / packetSize
    };
    sourceVoice->SubmitSourceBuffer(&buffer, &bufWma);
  }
  // �T�E���h�f�[�^�̓ǂݍ��݈ʒu���X�V.
  ol.Offset += transferedBytes;
  currentPos += transferedBytes;
  curBuf = (curBuf + 1) % MAX_BUFFER_COUNT;
  if (loop && currentPos >= dataSize) {
    ol.Offset = dataOffset;
    currentPos = 0;
  }
  hasAsyncRead = false;
}

/**
* Engine�̎���.
*/
class EngineImpl : public Engine
{
public:
  //	EngineImpl() : Engine(), xaudio(), masteringVoice(nullptr) {}
  //	virtual ~EngineImpl() {}

  virtual bool Initialize() override {
    ComPtr<IXAudio2> tmpAudio;
    UINT32 flags = 0;
#ifndef NDEBUG
    //		flags |= XAUDIO2_DEBUG_ENGINE;
#endif // NDEBUG
    HRESULT hr = XAudio2Create(&tmpAudio, flags);
    if (FAILED(hr)) {
      return false;
    }
    if (1) {
      XAUDIO2_DEBUG_CONFIGURATION debug = {};
      debug.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS | XAUDIO2_LOG_MEMORY;
      debug.BreakMask = XAUDIO2_LOG_ERRORS;
      debug.LogFunctionName = TRUE;
      tmpAudio->SetDebugConfiguration(&debug);
    }
    hr = tmpAudio->CreateMasteringVoice(&masteringVoice);
    if (FAILED(hr)) {
      return false;
    }
    xaudio.Swap(std::move(tmpAudio));
    return true;
  }

  virtual void Destroy() override {
    for (auto& e : streamSoundList) {
      if (e->hasAsyncRead) {
        CancelIoEx(e->handle, &e->ol);
      }
    }
    streamSoundList.clear();
    soundList.clear();
    xaudio.Reset();
  }

  virtual bool Update() override {
    soundList.remove_if(
      [](const SoundList::value_type& p) { return (p.use_count() <= 1) && (p->GetState() & State_Stopped); }
    );
    for (auto& e : streamSoundList) {
      e->Update();
    }
    streamSoundList.remove_if(
      [](const StreamSoundList::value_type& p) { return (p.use_count() <= 1) && (p->GetState() & State_Stopped); }
    );
    return true;
  }

  virtual SoundPtr Prepare(const wchar_t* filename) override {
    ScopedHandle hFile = CreateFile2(filename, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, nullptr);
    if (!hFile) {
      return nullptr;
    }
    WaveFormatInfo wf;
    std::shared_ptr<SoundImpl> sound(new SoundImpl);
    if (!LoadWaveFile(hFile, wf, sound->seekTable, &sound->source)) {
      return nullptr;
    }
    if (FAILED(xaudio->CreateSourceVoice(&sound->sourceVoice, &wf.u.ext.Format))) {
      return nullptr;
    }
    soundList.push_back(sound);
    return sound;
  }

  virtual SoundPtr PrepareStream(const wchar_t* filename) override {
    CREATEFILE2_EXTENDED_PARAMETERS ex = {
      sizeof(CREATEFILE2_EXTENDED_PARAMETERS),
      FILE_ATTRIBUTE_NORMAL,
      FILE_FLAG_OVERLAPPED,
      0,
      nullptr,
      nullptr
    };
    StreamSoundList::value_type p(new StreamSoundImpl(CreateFile2(filename, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &ex)));
    if (!p->handle) {
      return nullptr;
    }
    WaveFormatInfo wf;
    {
      ScopedHandle h = CreateFile2(filename, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, nullptr);
      if (!LoadWaveFile(h, wf, p->seekTable, nullptr)) {
        return nullptr;
      }
    }
    if (FAILED(xaudio->CreateSourceVoice(&p->sourceVoice, &wf.u.ext.Format))) {
      return nullptr;
    }
    p->ol.Offset = wf.dataOffset;
    p->ol.OffsetHigh = 0;
    p->dataOffset = wf.dataOffset;
    p->dataSize = wf.dataSize;
    p->packetSize = wf.u.ext.Format.nBlockAlign;
    streamSoundList.push_back(p);
    return p;
  }

  virtual void SetMasterVolume(float vol) override {
    if (xaudio) {
      masteringVoice->SetVolume(vol);
    }
  }

private:
  ComPtr<IXAudio2> xaudio;
  IXAudio2MasteringVoice* masteringVoice;

  typedef std::list<std::shared_ptr<SoundImpl>> SoundList;
  typedef std::list<std::shared_ptr<StreamSoundImpl>> StreamSoundList;
  SoundList soundList;
  StreamSoundList streamSoundList;
};

Engine& Engine::Get()
{
  static EngineImpl engine;
  return engine;
}

} // namespace Audio
